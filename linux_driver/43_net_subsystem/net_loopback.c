#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/skbuff.h>
#include <linux/ethtool.h>
#include <linux/sysfs.h>
#include <linux/if_ether.h>
#include <linux/if_arp.h>
#include <linux/ip.h>
#include <linux/icmp.h>
#include <linux/in.h>
#include <net/checksum.h>
#include <linux/spinlock.h>
#include <linux/version.h>

#define DRV_NAME    "net_loopback"   /* 驱动名称 */
#define DRV_VERSION "1.0"            /* 驱动版本号 */

#define NAPI_BUDGET 64   /* 每次NAPI轮询最多处理64个包 */

/* 设备私有数据结构体 */
struct net_lb_priv {
    struct net_device *ndev;        /* 网络设备指针 */
    struct napi_struct napi;        /* NAPI轮询结构体 */
    struct sk_buff_head lb_queue;   /* 回环skb队列，TX包翻转后暂存于此 */
    spinlock_t lb_lock;             /* 保护回环队列的自旋锁 */
    struct net_device_stats stats;  /* 收发统计信息结构体 */
    bool debug;                     /* 报文详细打印开关 */
};

/*
 * 回环报文翻转：将TX报文转换为对端回复再回送RX
 *
 * 处理逻辑：
 *   - 以太网层：交换源/目的MAC
 *   - ARP：交换发送方/目标的IP+MAC，opcode请求变应答
 *   - IP：交换源/目的IP
 *   - ICMP：echo-request变echo-reply，并重算校验和
 *
 * 这样主机发出的请求会被伪装成对端的应答回送上来
 */
static struct sk_buff *net_lb_transform(struct net_lb_priv *priv,
                                        struct sk_buff *skb)
{
    /* 以太网头指针 */
    struct ethhdr *eth;
    /* 新建回送包指针 */
    struct sk_buff *nskb;
    /* MAC交换临时缓冲 */
    unsigned char tmp_mac[ETH_ALEN];
    /* IP交换临时变量 */
    __be32 tmp_ip;

    /* 完整拷贝原skb，拷贝后可安全修改 */
    nskb = skb_copy(skb, GFP_ATOMIC);
    if (!nskb)
        return NULL;

    /* 以太网层：交换源/目的 MAC */
    eth = (struct ethhdr *)nskb->data;              /* 指向数据起始处的以太网头 */
    memcpy(tmp_mac, eth->h_source, ETH_ALEN);       /* 保存源MAC到临时缓冲 */
    memcpy(eth->h_source, eth->h_dest, ETH_ALEN);   /* 源MAC <- 目的MAC */
    memcpy(eth->h_dest, tmp_mac, ETH_ALEN);         /* 目的MAC <- 原源MAC */

    /* 判断是否为ARP报文 */
    if (eth->h_proto == htons(ETH_P_ARP)) {
        /* ARP头 */
        struct arphdr *arp = (struct arphdr *)(nskb->data + ETH_HLEN);
        /* ARP中的发送方/目标IP指针 */
        __be32 *sip, *tip;
        /* ARP中的发送方/目标MAC指针 */
        u8 *sha, *tha;

        /* 仅处理以太网/IPv4 ARP，其他类型跳过 */
        if (arp->ar_hrd != htons(ARPHRD_ETHER) ||  /* 硬件类型非以太网 */
            arp->ar_pro != htons(ETH_P_IP))        /* 协议类型非IPv4 */
            goto done;

        /*
         *   ARP有效负载布局：
         *   ARP固定头  arp  8字节
         *   发送方MAC  sha  6字节
         *   发送方IP   sip  4字节
         *   目标方MAC  tha  6字节
         *   目标方IP   tip  4字节
        */

        /* 发送方MAC位于ARP头之后 */
        sha = (u8 *)(arp + 1);
        /* 发送方IP位于MAC之后 */
        sip = (__be32 *)(sha + ETH_ALEN);
        /* 目标MAC位于IP之后 */
        tha = (u8 *)sip + 4;
        /* 目标IP位于MAC之后 */
        tip = (__be32 *)(tha + ETH_ALEN);

        /* opcode：请求变应答 */
        if (arp->ar_op == htons(ARPOP_REQUEST))  /* 若为ARP请求 */
            arp->ar_op = htons(ARPOP_REPLY);     /* 改为ARP应答 */

        /* 交换发送方/目标MAC */
        memcpy(tmp_mac, sha, ETH_ALEN);   /* 保存发送方MAC */
        memcpy(sha, tha, ETH_ALEN);       /* 发送方MAC <- 目标MAC */
        memcpy(tha, tmp_mac, ETH_ALEN);   /* 目标MAC <- 原发送方MAC */

        /* 交换发送方/目标 IP */
        tmp_ip = *sip;  /* 保存发送方IP */
        *sip = *tip;    /* 发送方IP <- 目标IP */
        *tip = tmp_ip;  /* 目标IP <- 原发送方IP */

        goto done;      /* ARP处理完成，跳转返回 */
    }

    /* 判断是否为IPv4报文 */
    if (eth->h_proto == htons(ETH_P_IP)) {
        /* IP头 */
        struct iphdr *iph = (struct iphdr *)(nskb->data + ETH_HLEN);

        /* 交换源/目的IP */
        tmp_ip = iph->saddr;     /* 保存源IP */
        iph->saddr = iph->daddr; /* 源IP <- 目的IP */
        iph->daddr = tmp_ip;     /* 目的IP <- 原源IP */

        /* 重算IP头校验和 */
        iph->check = 0;                           /* 校验和字段清零 */
        iph->check = ip_fast_csum(iph, iph->ihl); /* 重算IP头校验和 */

        /* 判断是否为ICMP报文 */
        if (iph->protocol == IPPROTO_ICMP) {
            /* ICMP头位于IP头之后 */
            struct icmphdr *icmph = (struct icmphdr *)((u8 *)iph +
                                  (iph->ihl * 4));
            /* 仅翻转echo-request */
            if (icmph->type == ICMP_ECHO) {       /* 若为echo请求 */
                icmph->type = ICMP_ECHOREPLY;     /* 改为echo应答 */

                /* 重算ICMP校验和 */
                icmph->checksum = 0;              /* 校验和字段清零 */
                icmph->checksum = csum_fold(      /* 折叠为16位校验和 */
                csum_partial(icmph, nskb->len - ETH_HLEN -
                                iph->ihl * 4, 0)); /* 计算ICMP部分校验和 */
            }
        }
    }

done:
    /* 返回转换后的回送包 */
    return nskb;
}

/* 以太网协议号转可读名称 */
static const char *eth_proto_name(u16 proto)
{
    /* 根据协议号匹配 */
    switch (proto) {
    case ETH_P_IP:   return "IPv4";
    case ETH_P_ARP:  return "ARP";
    case ETH_P_IPV6: return "IPv6";
    case ETH_P_RARP: return "RARP";
    default:         return "Other";
    }
}

/* IP上层协议号转可读名称 */
static const char *ip_proto_name(u8 proto)
{
    /* 根据协议号匹配 */
    switch (proto) {
    case IPPROTO_ICMP: return "ICMP";
    case IPPROTO_TCP:  return "TCP";
    case IPPROTO_UDP:  return "UDP";
    default:           return "?";
    }
}

/* ICMP类型转可读名称 */
static const char *icmp_type_name(u8 type)
{
    /* 根据类型匹配 */
    switch (type) {
    case ICMP_ECHO:         return "Echo-Request";
    case ICMP_ECHOREPLY:    return "Echo-Reply";
    case ICMP_DEST_UNREACH: return "Dest-Unreach";
    default:                return "Other";
    }
}

/* ARP opcode转可读名称 */
static const char *arp_op_name(u16 op)
{
    /* 根据操作码匹配 */
    switch (op) {
    case ARPOP_REQUEST: return "Request";
    case ARPOP_REPLY:   return "Reply";
    default:            return "Other";
    }
}

/*
 * 直接输出单个字段行：偏移 名称 原始字节(十六进制) 解码值
 * 每行单独printk，避免单次printk超过内核LOG_LINE_MAX(~1000字节)被截断
 */
static void net_lb_field_line(int offset, const char *name,
                             const u8 *data, int len,
                             const char *decoded)
{
    /* hex字节串缓冲 */
    char hex[64];
    /* hex写入偏移 */
    int i, h = 0;

    /* 拼接每个字节的十六进制，空格分隔 */
    for (i = 0; i < len && i < 16; i++)
        h += scnprintf(hex + h, sizeof(hex) - h,
                       i == 0 ? "%02x" : " %02x", data[i]);

    /* 超过16字节则用省略号标记 */
    if (len > 16)
        h += scnprintf(hex + h, sizeof(hex) - h, " ...");

    /* 逐行直接printk输出，保证长报文字段不被截断 */
    printk(KERN_INFO "  %04x  %-12s %-24s %s\n",
           offset, name, hex, decoded ? decoded : "");
}

/* 打印报文详细信息 */
static void net_lb_dump_skb(struct net_device *ndev,
                            const char *dir,
                            struct sk_buff *skb)
{
    /* 获取私有数据 */
    struct net_lb_priv *priv = netdev_priv(ndev);
    /* 指向以太网头 */
    struct ethhdr *eth = (struct ethhdr *)skb->data;
    /* 网络字节序转主机字节序协议号 */
    u16 proto = ntohs(eth->h_proto);
    /* 概要缓冲区：仅放标题与各协议单行摘要 */
    char hdr[512];
    /* 缓冲区已写入偏移量 */
    int n = 0;

    /* debug开关关闭时不打印 */
    if (!priv->debug)
        return;

    /* 标题行：方向、接口名、报文长度 */
    n += scnprintf(hdr + n, sizeof(hdr) - n,
        "\n---[%s]-- %s -- %u bytes --\n",
        dir, ndev->name, skb->len);

    /* 以太网层：目的MAC、源MAC、协议名称 */
    n += scnprintf(hdr + n, sizeof(hdr) - n,
        "| ETH   dst=%pM  src=%pM  %s\n",
        eth->h_dest, eth->h_source, eth_proto_name(proto));

    /* ARP报文解析 */
    if (proto == ETH_P_ARP) {  /* 若为ARP协议 */
        struct arphdr *arp = (struct arphdr *)(skb->data + ETH_HLEN); /* ARP 头 */
        u8 *sha = (u8 *)(arp + 1);                /* 发送方MAC */
        __be32 *sip = (__be32 *)(sha + ETH_ALEN); /* 发送方IP */
        u8 *tha = (u8 *)sip + 4;                  /* 目标MAC */
        __be32 *tip = (__be32 *)(tha + ETH_ALEN); /* 目标IP */

        n += scnprintf(hdr + n, sizeof(hdr) - n,     /* 追加ARP层信息 */
            "| ARP   %s  %pI4(%pM) --> %pI4(%pM)\n", /* 格式：操作码 IP(MAC) */
            arp_op_name(ntohs(arp->ar_op)),          /* ARP操作码名称 */
            sip, sha, tip, tha);                     /* IP和MAC参数 */
    }

    /* IP报文解析 */
    if (proto == ETH_P_IP) {  /* 若为IPv4协议 */
        struct iphdr *iph = (struct iphdr *)(skb->data + ETH_HLEN); /* IP头 */

        n += scnprintf(hdr + n, sizeof(hdr) - n,           /* 追加IP层信息 */
            "| IP    %pI4 --> %pI4  %s  ttl=%u  len=%u\n", /* 格式：源IP 目的IP */
            &iph->saddr, &iph->daddr,                      /* 源/目的IP */
            ip_proto_name(iph->protocol),                  /* 上层协议名称 */
            iph->ttl, ntohs(iph->tot_len));                /* TTL和总长度 */

        /* ICMP报文解析 */
        if (iph->protocol == IPPROTO_ICMP) {  /* 若上层为ICMP */
            struct icmphdr *icmph = (struct icmphdr *)((u8 *)iph +
                                  iph->ihl * 4); /* ICMP头位于IP头之后 */

            n += scnprintf(hdr + n, sizeof(hdr) - n,  /* 追加ICMP层信息 */
                "| ICMP  %s  id=0x%04x  seq=%u\n",    /* 格式：类型名 id seq */
                icmp_type_name(icmph->type),          /* ICMP类型名称 */
                ntohs(icmph->un.echo.id),             /* ICMP标识符 */
                ntohs(icmph->un.echo.sequence));      /* ICMP序列号 */
        }
    }

    /* 解析标题 */
    n += scnprintf(hdr + n, sizeof(hdr) - n, "--- fields ---\n");

    /* 概要一次输出 */
    netdev_info(ndev, "%s", hdr);

    /* 以下字段行逐行printk输出，避免单次printk过长被截断 */

    /* 以太网层字段：目的MAC、源MAC、以太类型 */
    {
        /* 临时解码值缓冲 */
        char val[40];

        scnprintf(val, sizeof(val), "%pM", eth->h_dest);  /* 目的MAC */
        net_lb_field_line(0, "dst-mac", eth->h_dest, ETH_ALEN, val);

        scnprintf(val, sizeof(val), "%pM", eth->h_source); /* 源MAC */
        net_lb_field_line(ETH_ALEN, "src-mac",
                          eth->h_source, ETH_ALEN, val);

        scnprintf(val, sizeof(val), "0x%04x (%s)",  /* 以太类型 */
                  proto, eth_proto_name(proto));
        net_lb_field_line(2 * ETH_ALEN, "ethertype",
                          (u8 *)&eth->h_proto, 2, val);
    }

    /* ARP报文字段解析 */
    if (proto == ETH_P_ARP) {
        /* ARP头 */
        struct arphdr *arp = (struct arphdr *)(skb->data + ETH_HLEN);
        /* 发送方/目标MAC、IP指针 */
        u8 *sha = (u8 *)(arp + 1);
        __be32 *sip = (__be32 *)(sha + ETH_ALEN);
        u8 *tha = (u8 *)sip + 4;
        __be32 *tip = (__be32 *)(tha + ETH_ALEN);
        /* 解码后的主机序字段值 */
        u16 ar_hrd = ntohs(arp->ar_hrd);
        u16 ar_pro = ntohs(arp->ar_pro);
        u16 ar_op = ntohs(arp->ar_op);
        /* 临时解码值缓冲 */
        char val[64];

        scnprintf(val, sizeof(val), "0x%04x (%s)", ar_hrd,  /* 硬件类型 */
                  ar_hrd == ARPHRD_ETHER ? "Ethernet" : "?");
        net_lb_field_line(ETH_HLEN, "hrd-type",
                          (u8 *)&arp->ar_hrd, 2, val);

        scnprintf(val, sizeof(val), "0x%04x (%s)", ar_pro,  /* 协议类型 */
                  ar_pro == ETH_P_IP ? "IPv4" : "?");
        net_lb_field_line(ETH_HLEN + 2, "pro-type",
                          (u8 *)&arp->ar_pro, 2, val);

        scnprintf(val, sizeof(val), "%u", arp->ar_hln);  /* 硬件地址长度 */
        net_lb_field_line(ETH_HLEN + 4, "hrd-len",
                          &arp->ar_hln, 1, val);

        scnprintf(val, sizeof(val), "%u", arp->ar_pln);  /* 协议地址长度 */
        net_lb_field_line(ETH_HLEN + 5, "pro-len",
                          &arp->ar_pln, 1, val);

        scnprintf(val, sizeof(val), "0x%04x (%s)", ar_op,  /* 操作码 */
                  arp_op_name(ar_op));
        net_lb_field_line(ETH_HLEN + 6, "opcode",
                          (u8 *)&arp->ar_op, 2, val);

        scnprintf(val, sizeof(val), "%pM", sha);  /* 发送方MAC */
        net_lb_field_line(ETH_HLEN + 8, "sender-mac",
                          sha, ETH_ALEN, val);

        scnprintf(val, sizeof(val), "%pI4", sip);  /* 发送方IP */
        net_lb_field_line(ETH_HLEN + 14, "sender-ip",
                          (u8 *)sip, 4, val);

        scnprintf(val, sizeof(val), "%pM", tha);  /* 目标MAC */
        net_lb_field_line(ETH_HLEN + 18, "target-mac",
                          tha, ETH_ALEN, val);

        scnprintf(val, sizeof(val), "%pI4", tip);  /* 目标IP */
        net_lb_field_line(ETH_HLEN + 24, "target-ip",
                          (u8 *)tip, 4, val);
    }

    /* IPv4报文字段解析 */
    if (proto == ETH_P_IP) {
        /* IP头 */
        struct iphdr *iph = (struct iphdr *)(skb->data + ETH_HLEN);
        /* 临时解码值缓冲 */
        char val[64];

        scnprintf(val, sizeof(val), "v%u, ihl=%u (%u bytes)",  /* 版本/头长 */
                  iph->version, iph->ihl, iph->ihl * 4);
        net_lb_field_line(ETH_HLEN, "ver/ihl", (u8 *)iph, 1, val);

        scnprintf(val, sizeof(val), "0x%02x", iph->tos);  /* 服务类型 */
        net_lb_field_line(ETH_HLEN + 1, "tos", &iph->tos, 1, val);

        scnprintf(val, sizeof(val), "%u", ntohs(iph->tot_len));  /* 总长度 */
        net_lb_field_line(ETH_HLEN + 2, "tot-len",
                          (u8 *)&iph->tot_len, 2, val);

        scnprintf(val, sizeof(val), "0x%04x", ntohs(iph->id));  /* 标识符 */
        net_lb_field_line(ETH_HLEN + 4, "id",
                          (u8 *)&iph->id, 2, val);

        scnprintf(val, sizeof(val), "flags=0x%x frag=%u",  /* 标志/分片 */
                  ntohs(iph->frag_off) >> 13,
                  ntohs(iph->frag_off) & 0x1fff);
        net_lb_field_line(ETH_HLEN + 6, "flags/frag",
                          (u8 *)&iph->frag_off, 2, val);

        scnprintf(val, sizeof(val), "%u", iph->ttl);  /* TTL */
        net_lb_field_line(ETH_HLEN + 8, "ttl", &iph->ttl, 1, val);

        scnprintf(val, sizeof(val), "0x%02x (%s)", iph->protocol,  /* 上层协议 */
                  ip_proto_name(iph->protocol));
        net_lb_field_line(ETH_HLEN + 9, "protocol",
                          &iph->protocol, 1, val);

        scnprintf(val, sizeof(val), "0x%04x", ntohs(iph->check));  /* 头校验和 */
        net_lb_field_line(ETH_HLEN + 10, "checksum",
                          (u8 *)&iph->check, 2, val);

        scnprintf(val, sizeof(val), "%pI4", &iph->saddr);  /* 源IP */
        net_lb_field_line(ETH_HLEN + 12, "src-ip",
                          (u8 *)&iph->saddr, 4, val);

        scnprintf(val, sizeof(val), "%pI4", &iph->daddr);  /* 目的IP */
        net_lb_field_line(ETH_HLEN + 16, "dst-ip",
                          (u8 *)&iph->daddr, 4, val);

        /* ICMP报文字段解析 */
        if (iph->protocol == IPPROTO_ICMP) {
            /* ICMP头位于IP头之后 */
            struct icmphdr *icmph = (struct icmphdr *)((u8 *)iph +
                                  iph->ihl * 4);
            /* ICMP字段起始偏移 */
            int icmp_off = ETH_HLEN + iph->ihl * 4;

            scnprintf(val, sizeof(val), "0x%02x (%s)", icmph->type,  /* 类型 */
                      icmp_type_name(icmph->type));
            net_lb_field_line(icmp_off, "type",
                              &icmph->type, 1, val);

            scnprintf(val, sizeof(val), "0x%02x", icmph->code);  /* 代码 */
            net_lb_field_line(icmp_off + 1, "code",
                              &icmph->code, 1, val);

            scnprintf(val, sizeof(val), "0x%04x",  /* 校验和 */
                      ntohs(icmph->checksum));
            net_lb_field_line(icmp_off + 2, "checksum",
                              (u8 *)&icmph->checksum, 2, val);

            scnprintf(val, sizeof(val), "0x%04x",  /* 标识符 */
                      ntohs(icmph->un.echo.id));
            net_lb_field_line(icmp_off + 4, "id",
                              (u8 *)&icmph->un.echo.id, 2, val);

            scnprintf(val, sizeof(val), "%u",  /* 序列号 */
                      ntohs(icmph->un.echo.sequence));
            net_lb_field_line(icmp_off + 6, "seq",
                              (u8 *)&icmph->un.echo.sequence, 2, val);

            /* ICMP负载数据：逐行转储剩余字节，最多显示64字节 */
            {
                int payload_off = icmp_off + 8;     /* 负载起始偏移 */
                int payload_len = skb->len - payload_off;  /* 负载总字节 */
                int show = min_t(int, payload_len, 64);    /* 最多64字节 */
                int i;

                /* 每16字节一行，逐行输出负载原始字节 */
                for (i = 0; i < show; i += 16) {
                    /* 本行字节数 */
                    int chunk = min_t(int, show - i, 16);

                    scnprintf(val, sizeof(val),
                              "(payload %d-%d/%d bytes)",
                              i, i + chunk, payload_len);
                    net_lb_field_line(payload_off + i,
                                      i == 0 ? "payload" : "cont",
                                      skb->data + payload_off + i,
                                      chunk, val);
                }
                /* 超过显示上限则提示总字节数 */
                if (payload_len > show) {
                    scnprintf(val, sizeof(val),
                              "(truncated, %d total bytes)",
                              payload_len);
                    net_lb_field_line(payload_off + show, "trunc",
                                      skb->data + payload_off + show,
                                      0, val);
                }
            }
        }
    }
}

/* NAPI轮询函数：从回环队列取出报文，回送网络栈 */
static int net_lb_napi_poll(struct napi_struct *napi, int budget)
{
    /* 取私有数据 */
    struct net_lb_priv *priv = container_of(napi, struct net_lb_priv, napi);
    /* 待回送的报文指针 */
    struct sk_buff *skb;
    /* 报文长度 */
    unsigned int pkt_len;
    /* 已处理报文计数 */
    int work_done = 0;

    /* 循环处理回环队列 */
    while (work_done < budget) {                /* 未达预算时继续处理 */
        spin_lock(&priv->lb_lock);              /* 加锁保护队列操作 */
        skb = __skb_dequeue(&priv->lb_queue);   /* 从队列头部取出一个skb */
        spin_unlock(&priv->lb_lock);            /* 解锁 */

        /* 取出为空，退出循环 */
        if (!skb)
            break;

        /* 设置报文关联到本网络设备 */
        skb->dev = priv->ndev;

        /* 打印RX报文详情 */
        net_lb_dump_skb(priv->ndev, "RX", skb);

        /* 解析以太网协议类型，设置skb->protocol和pkt_type */
        skb->protocol = eth_type_trans(skb, priv->ndev);
        /* 重置校验和，标记无需硬件校验 */
        skb->ip_summed = CHECKSUM_UNNECESSARY;

        /* 保存长度到局部变量 */
        pkt_len = skb->len;

        /* 回送报文到网络协议栈，模拟硬件接收，此后skb由协议栈释放 */
        netif_receive_skb(skb);

        /* 更新接收统计 */
        priv->stats.rx_packets++;        /* 接收包数+1 */
        priv->stats.rx_bytes += pkt_len; /* 接收字节累加 */
        work_done++;                     /* 已处理数+1 */
    }

    /* 若处理完毕且队列已空，结束NAPI轮询 */
    if (work_done < budget) {
        /* 完成NAPI并报告处理量 */
        napi_complete_done(napi, work_done);
    }

    /* 返回实际处理的报文数 */
    return work_done;
}

/* 打开网络设备：ifconfig up/ip link set up时调用 */
static int net_lb_ndo_open(struct net_device *ndev)
{
    /* 获取私有数据 */
    struct net_lb_priv *priv = netdev_priv(ndev);

    /* 启用NAPI轮询 */
    napi_enable(&priv->napi);
    /* 开启载波 */
    netif_carrier_on(ndev);
    /* 启动发送队列 */
    netif_start_queue(ndev);

    /* 打印打开信息 */
    netdev_info(ndev, "虚拟回环网卡已打开\n");

    return 0;
}

/* 关闭网络设备：ifconfig down/ip link set down时调用 */
static int net_lb_ndo_stop(struct net_device *ndev)
{
    /* 获取私有数据 */
    struct net_lb_priv *priv = netdev_priv(ndev);

    /* 停止发送队列 */
    netif_stop_queue(ndev);
    /* 关闭载波 */
    netif_carrier_off(ndev);
    /* 禁用NAPI */
    napi_disable(&priv->napi);

    /* 清空回环队列中残留报文 */
    skb_queue_purge(&priv->lb_queue);

    /* 打印关闭信息 */
    netdev_info(ndev, "虚拟回环网卡已关闭\n");

    return 0;
}

/* 发送报文：网络栈下发数据包时调用，实现回环 */
static netdev_tx_t net_lb_ndo_start_xmit(struct sk_buff *skb,
                                         struct net_device *ndev)
{
    /* 获取私有数据 */
    struct net_lb_priv *priv = netdev_priv(ndev);
    /* 回送用的转换报文 */
    struct sk_buff *nskb;

    /* debug模式下打印发送报文详情 */
    net_lb_dump_skb(ndev, "TX", skb);

    /* 更新发送统计 */
    priv->stats.tx_packets++;         /* 发送包数 +1 */
    priv->stats.tx_bytes += skb->len; /* 发送字节累加 */

    /* 将TX报文翻转为对端应答用于回送 */
    nskb = net_lb_transform(priv, skb);
    /* 释放原始发送skb */
    dev_kfree_skb(skb);

    /* 转换失败则直接返回 */
    if (!nskb) {
        priv->stats.tx_dropped++;   /* 丢包计数+1 */
        return NETDEV_TX_OK;        /* 返回发送成功 */
    }

    /* 将回送报文入队 */
    spin_lock(&priv->lb_lock);               /* 加锁保护队列 */
    __skb_queue_tail(&priv->lb_queue, nskb); /* 回送包加入队列尾部 */
    spin_unlock(&priv->lb_lock);             /* 解锁 */

    /* 调度NAPI轮询，触发回送收包 */
    napi_schedule(&priv->napi);

    /* 返回发送成功 */
    return NETDEV_TX_OK;
}

/* 获取网络统计信息 */
static struct net_device_stats *net_lb_ndo_get_stats(struct net_device *ndev)
{
    /* 获取私有数据 */
    struct net_lb_priv *priv = netdev_priv(ndev);

    /* 返回统计结构指针 */
    return &priv->stats;
}

/* 设置MAC地址 */
static int net_lb_ndo_set_mac_address(struct net_device *ndev, void *addr)
{
    /* 调用通用以太网MAC设置函数，内部完成校验与dev_addr_set */
    return eth_mac_addr(ndev, addr);
}

/* net_device_ops操作集 */
static const struct net_device_ops net_lb_ops = {
    .ndo_open               = net_lb_ndo_open,            /* 打开设备回调 */
    .ndo_stop               = net_lb_ndo_stop,            /* 关闭设备回调 */
    .ndo_start_xmit         = net_lb_ndo_start_xmit,      /* 发送报文回调 */
    .ndo_get_stats          = net_lb_ndo_get_stats,       /* 获取统计回调 */
    .ndo_set_mac_address    = net_lb_ndo_set_mac_address, /* 设置MAC回调 */
    .ndo_validate_addr      = eth_validate_addr,          /* 校验MAC回调，使用内核通用函数 */
};

/* 获取驱动信息 */
static void net_lb_et_get_drvinfo(struct net_device *ndev,
                                  struct ethtool_drvinfo *info)
{   
    /* 填入驱动名 */
    strscpy(info->driver, DRV_NAME, sizeof(info->driver));
    /* 填入版本号 */
    strscpy(info->version, DRV_VERSION, sizeof(info->version));
    /* 填入总线信息 */
    strscpy(info->bus_info, "virtual", sizeof(info->bus_info));
}

/* 获取链路状态 */
static u32 net_lb_et_get_link(struct net_device *ndev)
{
    /* 载波正常返回1，否则0 */
    return netif_carrier_ok(ndev) ? 1 : 0;
}

/* 统计信息字符串表 */
static const char net_lb_stat_strings[][ETH_GSTRING_LEN] = {
    "tx_packets", "tx_bytes", "rx_packets", "rx_bytes",  /* 发送数据包数、发送字节数、接收数据包数、接收字节数 */
    "tx_dropped", "lb_queue_len",                        /* 丢包数、队列长度 */
};

/* 获取统计字符串集 */
static void net_lb_et_get_strings(struct net_device *ndev, u32 stringset,
                                  u8 *data)
{
    /* 若请求统计字符串 */
    if (stringset == ETH_SS_STATS)
        memcpy(data, net_lb_stat_strings, sizeof(net_lb_stat_strings)); /* 拷贝字符串表 */
}

/* 获取统计项数量 */
static int net_lb_et_get_sset_count(struct net_device *ndev, int sset)
{
    /* 若为统计字符串集 */
    if (sset == ETH_SS_STATS)
        return ARRAY_SIZE(net_lb_stat_strings);  /* 返回数组元素个数 */

    return 0;
}

/* 获取统计数据 */
static void net_lb_et_get_ethtool_stats(struct net_device *ndev,
                                        struct ethtool_stats *stats,
                                        u64 *data)
{
    /* 获取私有数据 */
    struct net_lb_priv *priv = netdev_priv(ndev);

    data[0] = priv->stats.tx_packets;          /* 发送包数 */
    data[1] = priv->stats.tx_bytes;            /* 发送字节 */
    data[2] = priv->stats.rx_packets;          /* 接收包数 */
    data[3] = priv->stats.rx_bytes;            /* 接收字节 */
    data[4] = priv->stats.tx_dropped;          /* 发送丢包数 */
    data[5] = skb_queue_len(&priv->lb_queue);  /* 当前回环队列长度 */
}

/* ethtool操作集 */
static const struct ethtool_ops net_lb_ethtool_ops = {
    .get_drvinfo       = net_lb_et_get_drvinfo,        /* 获取驱动信息 */
    .get_link          = net_lb_et_get_link,           /* 获取链路状态 */
    .get_strings       = net_lb_et_get_strings,        /* 获取统计字符串 */
    .get_sset_count    = net_lb_et_get_sset_count,     /* 获取统计项数 */
    .get_ethtool_stats = net_lb_et_get_ethtool_stats,  /* 获取统计数据 */
};

/* sysfs属性：显示debug开关状态 */
static ssize_t debug_show(struct device *dev,
                          struct device_attribute *attr, char *buf)
{
    /* 从device取net_device */
    struct net_device *ndev = to_net_dev(dev);
    /* 获取私有数据 */
    struct net_lb_priv *priv = netdev_priv(ndev);

    /* 输出0=关 1=开 */
    return sprintf(buf, "%d\n", priv->debug ? 1 : 0);
}

/* sysfs属性：设置debug开关 */
static ssize_t debug_store(struct device *dev,
                           struct device_attribute *attr,
                           const char *buf, size_t count)
{
    /* 从device取net_device */
    struct net_device *ndev = to_net_dev(dev);
    /* 获取私有数据 */
    struct net_lb_priv *priv = netdev_priv(ndev);
    /* 解析后的数值 */
    unsigned long val;

    /* 将字符串解析为无符号长整型 */
    if (kstrtoul(buf, 0, &val))
        return -EINVAL;

    /* 非0即开启，0为关闭 */
    priv->debug = val ? true : false;
    
    /* 打印状态 */
    netdev_info(ndev, "报文详细打印: %s\n", priv->debug ? "开启" : "关闭");

    /* 返回已写入字节数 */
    return count;
}

/* 读写属性宏 */
static DEVICE_ATTR_RW(debug);

/* sysfs属性指针数组 */
static struct attribute *net_lb_attrs[] = {
    &dev_attr_debug.attr,     /* debug开关属性 */
    NULL,
};

/* sysfs属性组 */
static const struct attribute_group net_lb_attr_group = {
    .name = "loopback",        /* 属性组目录名 */
    .attrs = net_lb_attrs,     /* 属性指针数组 */
};

/* 全局网络设备指针 */
static struct net_device *lb_ndev;

/* 网络设备初始化 */
static void net_lb_netdev_setup(struct net_device *ndev)
{
    /* 获取私有数据指针 */
    struct net_lb_priv *priv = netdev_priv(ndev);

    /* 初始化以太网标准字段 */
    ether_setup(ndev);

    /* 保存网络设备指针到私有数据 */
    priv->ndev = ndev;

    /* 初始化回环队列自旋锁 */
    spin_lock_init(&priv->lb_lock);
    /* 初始化回环skb队列 */
    skb_queue_head_init(&priv->lb_queue);

    /* 绑定net_device操作回调 */
    ndev->netdev_ops = &net_lb_ops;
    /* 绑定ethtool操作回调 */
    ndev->ethtool_ops = &net_lb_ethtool_ops;

    /* 设置设备特性 */
    ndev->features = 0;    /* 无硬件特性 */
    ndev->hw_features = 0; /* 无可切换硬件特性 */

    /* 设置MTU */
    ndev->mtu = ETH_DATA_LEN;  /* 标准以太网MTU 1500字节 */

    /* 设置需要的最小头空间 */
    ndev->needed_headroom = NET_IP_ALIGN;  /* 2字节对齐头空间 */

    /* 生成随机MAC地址 */
    eth_hw_addr_random(ndev);

    /* 初始化NAPI */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0)
    netif_napi_add(ndev, &priv->napi, net_lb_napi_poll);
#else
    netif_napi_add(ndev, &priv->napi, net_lb_napi_poll, NAPI_POLL_WEIGHT);
#endif
}

/* 模块加载函数 */
static int __init net_lb_init(void)
{
    /* 返回值 */
    int ret;

    /* 打印加载信息 */
    pr_info("虚拟回环网卡加载\n");

    /* 分配网络设备，命名为lbnetX */
    lb_ndev = alloc_netdev(sizeof(struct net_lb_priv), "lbnet%d",
                           NET_NAME_UNKNOWN, net_lb_netdev_setup);
    if (!lb_ndev) {
        pr_err("alloc_netdev 失败\n");
        return -ENOMEM;
    }

    /* 注册网络设备到内核 */
    ret = register_netdev(lb_ndev);
    if (ret) {
        pr_err("register_netdev 失败: %d\n", ret);
        goto err_free_netdev;
    }

    /* 创建sysfs属性组，在/sys/class/net/<dev>/loopback/下 */
    ret = sysfs_create_group(&lb_ndev->dev.kobj, &net_lb_attr_group);
    if (ret) {
        pr_err("sysfs 属性组创建失败: %d\n", ret);
        goto err_unregister_netdev;
    }

    return 0;

err_unregister_netdev:
    /* 注销已注册的设备 */
    unregister_netdev(lb_ndev);
err_free_netdev:
    /* 释放设备内存 */
    free_netdev(lb_ndev);
    /* 置空指针 */
    lb_ndev = NULL;
    /* 返回错误码 */
    return ret;
}

/* 模块卸载函数 */
static void __exit net_lb_exit(void)
{
    /* 获取私有数据 */
    struct net_lb_priv *priv = netdev_priv(lb_ndev);

    /* 打印卸载信息 */
    pr_info("虚拟回环网卡卸载\n");

    /* 移除sysfs属性组 */
    sysfs_remove_group(&lb_ndev->dev.kobj, &net_lb_attr_group);

    /* 注销网络设备 */
    unregister_netdev(lb_ndev);

    /* 移除NAPI */
    netif_napi_del(&priv->napi);

    /* 释放网络设备 */
    free_netdev(lb_ndev);
}

module_init(net_lb_init);
module_exit(net_lb_exit);

MODULE_AUTHOR("embedfire <embedfire@embedfire.com>");
MODULE_DESCRIPTION("net_loopback module");
MODULE_LICENSE("GPL v2");