#include <linux/module.h>
#include <linux/init.h>
#include <linux/serial_core.h>
#include <linux/serial.h>
#include <linux/tty.h>
#include <linux/tty_flip.h>
#include <linux/timer.h>
#include <linux/spinlock.h>
#include <linux/uaccess.h>
#include <linux/ioctl.h>
#include <linux/slab.h>
#include <linux/version.h>
#include <linux/serial_reg.h>

#define TTYTEST_NR_PORTS    1            /* 串口数量 */
#define TTYTEST_FIFO_SIZE   16           /* FIFO大小，字节 */
#define TTYTEST_RX_BUF_SIZE 512          /* 接收缓冲区大小 */
#define TTYTEST_TIMER_PERIOD  (2 * HZ)   /* 定时器周期，2秒 */

#define DEFAULT_RX_MSG        "Virtual UART: Hello from 16550!\n"    /* 默认接收消息 */

/* 驱动私有数据结构体 */
struct ttytest_port {
    struct uart_port port;  /* UART端口基础结构 */
    spinlock_t lock;        /* 自旋锁，保护寄存器访问 */

    u8 regs[8];     /* 虚拟UART寄存器数组 */
    u16 divisor;    /* 波特率除数 */
    u8 lcr;         /* 线路控制寄存器缓存 */

    struct timer_list rx_timer;         /* 接收定时器，模拟数据接收 */
    char rx_buf[TTYTEST_RX_BUF_SIZE];   /* 接收数据缓冲区 */
    int rx_len;                         /* 接收数据长度 */

    char tx_accum[TTYTEST_RX_BUF_SIZE]; /* 发送数据累积缓冲区 */
    int tx_accum_len;                   /* 累积数据长度 */

    unsigned int baudrate;  /* 当前波特率 */
    u8 data_bits;           /* 当前数据位数 */
    u8 parity;              /* 当前校验方式 */
    u8 stop_bits;           /* 当前停止位数 */
};

/* 从uart_port指针获取ttytest_port指针的宏 */
#define to_ttytest_port(p)    container_of(p, struct ttytest_port, port)

/* 全局端口实例指针 */
static struct ttytest_port *g_ttytest_port;

/* 定时器回调：模拟接收数据 */
static void ttytest_rx_timer(struct timer_list *t)
{
    /* 从定时器结构获取ttytest_port指针 */
    struct ttytest_port *tp = from_timer(tp, t, rx_timer);
    /* 获取UART端口指针 */
    struct uart_port *up = &tp->port;
    /* 保存中断状态的标志变量 */
    unsigned long flags;
    /* 循环计数器 */
    int i;

    /* 保存中断状态并获取自旋锁 */
    spin_lock_irqsave(&tp->lock, flags);

    /* 遍历接收缓冲区中的所有字符 */
    for (i = 0; i < tp->rx_len; i++) {
        /* 将字符插入到TTY翻转缓冲区 */
        /* 参数: UART端口, LSR状态, 错误掩码, 字符, TTY标志 */
        uart_insert_char(up, UART_LSR_DR, 0, tp->rx_buf[i], TTY_NORMAL);
    }

    /* 释放自旋锁并恢复中断状态 */
    spin_unlock_irqrestore(&tp->lock, flags);

    /* 将翻转缓冲区的数据推送到TTY子系统 */
    tty_flip_buffer_push(&up->state->port);

    /* 重新设置定时器，2秒后再次触发 */
    mod_timer(&tp->rx_timer, jiffies + TTYTEST_TIMER_PERIOD);
}

/* UART操作回调：检查发送器是否为空 */
static unsigned int ttytest_tx_empty(struct uart_port *port)
{
    /* 获取ttytest_port指针 */
    struct ttytest_port *tp = to_ttytest_port(port);
    /* 保存中断状态的标志变量 */
    unsigned long flags;
    /* 返回值 */
    unsigned int ret;

    /* 保存中断状态并获取自旋锁 */
    spin_lock_irqsave(&tp->lock, flags);

    /* 检查LSR线路状态寄存器的TEMT位(Bit6)判断发送器是否完全空 */
    ret = (tp->regs[UART_LSR] & UART_LSR_TEMT) ? TIOCSER_TEMT : 0;

    /* 释放自旋锁并恢复中断状态 */
    spin_unlock_irqrestore(&tp->lock, flags);

    return ret;
}

/*
* ttytest_set_mctrl - 设置调制解调器控制信号
* @port: UART端口指针
* @mctrl: 控制信号位图(TIOCM_DTR/TIOCM_RTS等)
*/
static void ttytest_set_mctrl(struct uart_port *port, unsigned int mctrl)
{
    /* 获取ttytest_port指针 */
    struct ttytest_port *tp = to_ttytest_port(port);
    /* 保存中断状态的标志变量 */
    unsigned long flags;
    /* 调制解调器控制寄存器值 */
    u8 mcr = 0;

    /* 保存中断状态并获取自旋锁 */
    spin_lock_irqsave(&tp->lock, flags);

    /* 将TIOCM控制信号转换为MCR调制解调器控制寄存器位 */
    if (mctrl & TIOCM_DTR)     mcr |= UART_MCR_DTR;    /* 数据就绪 */
    if (mctrl & TIOCM_RTS)     mcr |= UART_MCR_RTS;    /* 请求发送 */
    if (mctrl & TIOCM_OUT1)    mcr |= UART_MCR_OUT1;   /* 输出1 */
    if (mctrl & TIOCM_OUT2)    mcr |= UART_MCR_OUT2;   /* 输出2 */
    if (mctrl & TIOCM_LOOP)    mcr |= UART_MCR_LOOP;   /* 回环模式 */

    /* 更新MCR调制解调器控制寄存器 */
    tp->regs[UART_MCR] = mcr;

    /* 检查是否处于回环模式 (LOOP位) */
    if (mcr & UART_MCR_LOOP) {
        /* 回环模式：MCR输出直接反馈到MSR输入 */
        /* 必须先清除MSR中的相关状态位，防止状态残留 */
        tp->regs[UART_MSR] &= ~(UART_MSR_CTS | UART_MSR_DSR | UART_MSR_RI | UART_MSR_DCD);
        
        /* 16550硬件规范：在本地回环模式下，MCR的输出引脚在芯片内部直接短接到MSR的输入引脚 */
        if (mcr & UART_MCR_RTS)  tp->regs[UART_MSR] |= UART_MSR_CTS;  /* RTS(请求发送) 环回到 CTS(允许发送) */
        if (mcr & UART_MCR_DTR)  tp->regs[UART_MSR] |= UART_MSR_DSR;  /* DTR(终端就绪) 环回到 DSR(设备就绪) */
        if (mcr & UART_MCR_OUT1) tp->regs[UART_MSR] |= UART_MSR_RI;   /* OUT1(输出1) 环回到 RI(振铃指示) */
        if (mcr & UART_MCR_OUT2) tp->regs[UART_MSR] |= UART_MSR_DCD;  /* OUT2(输出2) 环回到 DCD(载波检测) */
    } else {
        /* 非回环模式：MSR反映外部状态。
         * 在虚拟驱动中，通常模拟一个“外部设备已连接且就绪”的状态，或者直接清零。
         * 这里模拟CTS/DSR/DCD始终有效，以保证正常通信，防止硬件流控死锁 */
        tp->regs[UART_MSR] = UART_MSR_CTS | UART_MSR_DSR | UART_MSR_DCD;
        /* RI振铃指示通常保持无效 */
        tp->regs[UART_MSR] &= ~UART_MSR_RI;
    }
    /* 释放自旋锁并恢复中断状态 */
    spin_unlock_irqrestore(&tp->lock, flags);
}

/*
* ttytest_get_mctrl - 获取调制解调器控制信号状态
* @port: UART端口指针
*
* 返回: 当前调制解调器控制信号状态
*/
static unsigned int ttytest_get_mctrl(struct uart_port *port)
{
    /* 获取ttytest_port指针 */
    struct ttytest_port *tp = to_ttytest_port(port);
    /* 保存中断状态的标志变量 */
    unsigned long flags;
    /* 返回的控制信号状态 */
    unsigned int mctrl = 0;
    /* 调制解调器状态寄存器值 */
    u8 msr;

    /* 保存中断状态并获取自旋锁 */
    spin_lock_irqsave(&tp->lock, flags);

    /* 读取MSR调制解调器状态寄存器 */
    msr = tp->regs[UART_MSR];

    /* 将MSR位转换为TIOCM控制信号 */
    if (msr & UART_MSR_CTS)    mctrl |= TIOCM_CTS;   /* 清除发送 */
    if (msr & UART_MSR_DSR)    mctrl |= TIOCM_DSR;   /* 数据就绪 */
    if (msr & UART_MSR_RI)     mctrl |= TIOCM_RI;    /* 振铃指示 */
    if (msr & UART_MSR_DCD)    mctrl |= TIOCM_CD;    /* 载波检测 */

    /* 清除MSR中的delta标志位 */
    tp->regs[UART_MSR] &= ~(UART_MSR_DDCD | UART_MSR_TERI |
                UART_MSR_DDSR | UART_MSR_DCTS);

    /* 释放自旋锁并恢复中断状态 */
    spin_unlock_irqrestore(&tp->lock, flags);

    /* 返回结果 */
    return mctrl;
}

/*
* ttytest_stop_tx - 停止发送
* @port: UART端口指针
*/
static void ttytest_stop_tx(struct uart_port *port)
{
    /* 虚拟驱动无需额外操作 */
}

/*
* ttytest_start_tx - 开始发送
* @port: UART端口指针
*
* 从UART发送环形缓冲区读取数据，累积到tx_accum中。
* 当收到换行符时，比较累积内容与当前rx_buf，如果不同则更新rx_buf为新内容。
*/
static void ttytest_start_tx(struct uart_port *port)
{
    /* 获取ttytest_port指针 */
    struct ttytest_port *tp = to_ttytest_port(port);
    /* 获取UART发送环形缓冲区指针 */
    struct circ_buf *xmit = &port->state->xmit;
    /* 保存中断状态的标志变量 */
    unsigned long flags;
    /* 当前读取的字符 */
    unsigned char ch;

    /* 保存中断状态并获取自旋锁 */
    spin_lock_irqsave(&tp->lock, flags);

    /* 循环读取发送缓冲区中的数据 */
    while (!uart_circ_empty(xmit) && tp->tx_accum_len < TTYTEST_RX_BUF_SIZE - 1) {
        /* 从发送缓冲区读取一个字符 */
        ch = xmit->buf[xmit->tail];
        /* 移动发送缓冲区尾指针 */
        xmit->tail = (xmit->tail + 1) & (UART_XMIT_SIZE - 1);
        /* 增加发送计数 */
        port->icount.tx++;

        /* 跳过回车字符'\r'，避免规范模式下ONLCR转换导致的内容污染 */
        if (ch == '\r')
            continue;

        /* 将字符累积到tx_accum缓冲区 */
        tp->tx_accum[tp->tx_accum_len++] = ch;

        /* 检查是否收到换行符 */
        if (ch == '\n') {
            /* 添加字符串结束符 */
            tp->tx_accum[tp->tx_accum_len] = '\0';

            /* 比较累积内容与当前rx_buf */
            /* 如果内容不同，更新rx_buf为新内容 */
            if (strcmp(tp->tx_accum, tp->rx_buf) != 0) {
                memcpy(tp->rx_buf, tp->tx_accum, tp->tx_accum_len + 1);
                tp->rx_len = tp->tx_accum_len;
            }

            /* 重置累积缓冲区长度 */
            tp->tx_accum_len = 0;
        }
    }

    /* 设置LSR线路状态寄存器的THRE(Bit5)和TEMT位(Bit6)，表示发送器为空 */
    tp->regs[UART_LSR] |= UART_LSR_THRE | UART_LSR_TEMT;

    /* 释放自旋锁并恢复中断状态 */
    spin_unlock_irqrestore(&tp->lock, flags);

    /* 唤醒等待写入的进程 */
    uart_write_wakeup(port);
}

/*
* ttytest_stop_rx - 停止接收
* @port: UART端口指针
*/
static void ttytest_stop_rx(struct uart_port *port)
{
    /* 虚拟驱动无需额外操作 */
}

/*
* ttytest_start_rx - 开始接收
* @port: UART端口指针
*
* 设置接收定时器，2秒后触发模拟数据接收
*/
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 0, 0)
static void ttytest_start_rx(struct uart_port *port)
{
    /* 获取ttytest_port指针 */
    struct ttytest_port *tp = to_ttytest_port(port);

    /* 设置定时器，2秒后触发 */
    mod_timer(&tp->rx_timer, jiffies + TTYTEST_TIMER_PERIOD);
}
#endif

/*
* ttytest_break_ctl - 控制Break信号
* @port: UART端口指针
* @break_state: Break状态(非零表示开启Break，零表示关闭Break)
*/
static void ttytest_break_ctl(struct uart_port *port, int break_state)
{
    /* 获取ttytest_port指针 */
    struct ttytest_port *tp = to_ttytest_port(port);
    /* 保存中断状态的标志变量 */
    unsigned long flags;

    /* 保存中断状态并获取自旋锁 */
    spin_lock_irqsave(&tp->lock, flags);

    /* 根据break_state设置或清除LCR线路控制寄存器的SBC位(Bit6) */
    if (break_state)
        tp->lcr |= UART_LCR_SBC;     /* 开启Break */
    else
        tp->lcr &= ~UART_LCR_SBC;    /* 关闭Break */

    /* 更新LCR线路控制寄存器 */
    tp->regs[UART_LCR] = tp->lcr;

    /* 释放自旋锁并恢复中断状态 */
    spin_unlock_irqrestore(&tp->lock, flags);
}

/*
* ttytest_set_termios - 设置终端属性
* @port: UART端口指针
* @termios: 新的终端属性
* @old: 旧的终端属性
*
* 设置波特率、数据位、校验位、停止位等参数
*/
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 1, 0)
static void ttytest_set_termios(struct uart_port *port, struct ktermios *termios,
                                const struct ktermios *old)
#else
static void ttytest_set_termios(struct uart_port *port, struct ktermios *termios,
                                struct ktermios *old)
#endif
{
    /* 获取ttytest_port指针 */
    struct ttytest_port *tp = to_ttytest_port(port);
    /* 保存中断状态的标志变量 */
    unsigned long flags;
    /* 计算后的波特率 */
    unsigned int baud;
    /* 线路控制寄存器值 */
    u8 lcr = 0;

    /* 获取波特率，范围300-4000000 */
    baud = uart_get_baud_rate(port, termios, old, 300, 4000000);

    /* 保存中断状态并获取自旋锁 */
    spin_lock_irqsave(&tp->lock, flags);

    /* 保存当前波特率 */
    tp->baudrate = baud;

    /* 根据termios设置LCR线路控制寄存器数据位数(Bit1和Bit0) */
    switch (termios->c_cflag & CSIZE) {
    case CS5:    lcr |= UART_LCR_WLEN5;    tp->data_bits = 5;    break;    /* 5位数据 */
    case CS6:    lcr |= UART_LCR_WLEN6;    tp->data_bits = 6;    break;    /* 6位数据 */
    case CS7:    lcr |= UART_LCR_WLEN7;    tp->data_bits = 7;    break;    /* 7位数据 */
    case CS8:
    default:    lcr |= UART_LCR_WLEN8;    tp->data_bits = 8;    break;     /* 8位数据 */
    }

    /* 设置停止位数(Bit2) */
    if (termios->c_cflag & CSTOPB) {
        lcr |= UART_LCR_STOP;  /* 设置停止位为2位 */
        tp->stop_bits = 2;
    } else {
        tp->stop_bits = 1;    /* 默认1位停止位 */
    }

    /* 设置校验方式(Bit5~Bit3) */
    tp->parity = 0;                /* 默认无校验 */
    if (termios->c_cflag & PARENB) {
        lcr |= UART_LCR_PARITY;    /* 使能校验 */
        if (termios->c_cflag & PARODD) {
            tp->parity = 1;        /* 奇校验 */
        } else {
            lcr |= UART_LCR_EPAR;  /* 偶校验 */
            tp->parity = 2;
        }
    }

    /* 计算波特率除数：Divisior=输入时钟频率/(16×所需波特率) */
    if (baud > 0) {
        tp->divisor = 1843200 / (16 * baud);
    } else {
        tp->divisor = 0;
    }

    /* 保存LCR基础值 */
    tp->lcr = lcr;

    /* 设置LCR的DLAB位(Bit7)为1，将偏移0和1切换为除数锁存器 */
    tp->regs[UART_LCR] = lcr | UART_LCR_DLAB;

    /* 将波特率除数分别写入DLL(偏移0)和DLM(偏移1) */
    tp->regs[UART_DLL] = tp->divisor & 0xff;         /* 写入除数低 8 位 */
    tp->regs[UART_DLM] = (tp->divisor >> 8) & 0xff;  /* 写入除数高 8 位 */

    /* 清除DLAB位，将偏移0和1恢复为数据收发(RBR/THR)与中断使能(IER)寄存器，并写入最终的LCR配置 */
    tp->regs[UART_LCR] = lcr;

    /* 释放自旋锁并恢复中断状态 */
    spin_unlock_irqrestore(&tp->lock, flags);

    /* 更新超时参数 */
    uart_update_timeout(port, termios->c_cflag, baud);
}

/*
* ttytest_ioctl - 处理IO控制命令
* @port: UART端口指针
* @cmd: IO控制命令
* @arg: 命令参数
*
* 返回: -ENOIOCTLCMD表示不支持该命令
*/
static int ttytest_ioctl(struct uart_port *port, unsigned int cmd, unsigned long arg)
{
    /* 当前不支持任何IO控制命令 */
    return -ENOIOCTLCMD;
}

/*
* ttytest_startup - 启动UART端口
* @port: UART端口指针
*/
static int ttytest_startup(struct uart_port *port)
{
    /* 获取ttytest_port指针 */
    struct ttytest_port *tp = to_ttytest_port(port);
    /* 保存中断状态的标志变量 */
    unsigned long flags;

    /* 保存中断状态并获取自旋锁 */
    spin_lock_irqsave(&tp->lock, flags);

    /* 重置发送累积缓冲区长度 */
    tp->tx_accum_len = 0;

    /* 释放自旋锁并恢复中断状态 */
    spin_unlock_irqrestore(&tp->lock, flags);

    /* 设置接收定时器，2秒后触发 */
    mod_timer(&tp->rx_timer, jiffies + TTYTEST_TIMER_PERIOD);

    return 0;
}

/*
* ttytest_shutdown - 关闭UART端口
* @port: UART端口指针
*/
static void ttytest_shutdown(struct uart_port *port)
{
    /* 获取ttytest_port指针 */
    struct ttytest_port *tp = to_ttytest_port(port);

    /* 删除定时器并等待其完成 */
    del_timer_sync(&tp->rx_timer);
}

/* UART操作集 */
static const struct uart_ops ttytest_uart_ops = {
    .tx_empty    = ttytest_tx_empty,    /* 检查发送器是否为空 */
    .set_mctrl   = ttytest_set_mctrl,   /* 设置调制解调器控制信号 */
    .get_mctrl   = ttytest_get_mctrl,   /* 获取调制解调器控制信号 */
    .stop_tx     = ttytest_stop_tx,     /* 停止发送 */
    .start_tx    = ttytest_start_tx,    /* 开始发送 */
    .stop_rx     = ttytest_stop_rx,     /* 停止接收 */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 0, 0)
    .start_rx    = ttytest_start_rx,    /* 开始接收 */
#endif
    .break_ctl   = ttytest_break_ctl,   /* 控制中断信号 */
    .set_termios = ttytest_set_termios, /* 设置终端属性 */
    .ioctl       = ttytest_ioctl,       /* 处理IO控制命令 */
    .startup     = ttytest_startup,     /* 启动UART端口 */
    .shutdown    = ttytest_shutdown,    /* 关闭UART端口 */
};

/* UART驱动结构体 */
static struct uart_driver ttytest_driver = {
    .owner        = THIS_MODULE,
    .driver_name  = "tty_subsystem",    /* 驱动名称 */
    .dev_name     = "ttyTEST",          /* 设备接口名称 */
    .major        = 0,                  /* 主设备号，使用自动分配 */
    .minor        = 0,                  /* 次设备号起始值 */
    .nr           = TTYTEST_NR_PORTS,   /* 串口数量 */
};

/* 模块初始化函数 */
static int __init ttytest_init(void)
{
    /* 返回值 */
    int ret;
    /* 端口实例指针 */
    struct ttytest_port *tp;

    /* 注册UART驱动 */
    ret = uart_register_driver(&ttytest_driver);
    if (ret) {
        pr_err("ttytest: uart_register_driver failed, ret=%d\n", ret);
        return ret;
    }

    /* 分配端口实例内存 */
    tp = kzalloc(sizeof(struct ttytest_port), GFP_KERNEL);
    if (!tp) {
        ret = -ENOMEM;
        goto err_unreg_driver;
    }

    /* 保存全局端口实例指针 */
    g_ttytest_port = tp;

    /* 初始化自旋锁 */
    spin_lock_init(&tp->lock);

    /* 设置UART操作集指针 */
    tp->port.ops = &ttytest_uart_ops;
    /* 设置端口号为0 */
    tp->port.line = 0;
    /* 设置UART类型为16550A */
    tp->port.type = PORT_16550A;
    /* 设置IO类型为内存映射 */
    tp->port.iotype = UPIO_MEM;
    /* 设置UART时钟频率为1.8432MHz  */
    tp->port.uartclk = 1843200;
    /* 设置FIFO大小为16字节 */
    tp->port.fifosize = TTYTEST_FIFO_SIZE;
    /* 设置端口标志 */
    tp->port.flags = UPF_BOOT_AUTOCONF;

    /* 初始化LSR线路状态寄存器：发送保持寄存器空、发送器空 */
    tp->regs[UART_LSR] = UART_LSR_THRE | UART_LSR_TEMT;
    /* 初始化MSR调制解调器状态寄存器：CTS、DSR、DCD有效 */
    tp->regs[UART_MSR] = UART_MSR_CTS | UART_MSR_DSR | UART_MSR_DCD;
    /* 初始化MCR调制解调器控制寄存器：DTR、RTS有效 */
    tp->regs[UART_MCR] = UART_MCR_DTR | UART_MCR_RTS;
    /* 初始化FIFO控制寄存器：使能FIFO、清除接收FIFO、清除发送FIFO、接收触发级别8字节 */
    tp->regs[UART_FCR] = UART_FCR_ENABLE_FIFO |
                UART_FCR_CLEAR_RCVR |
                UART_FCR_CLEAR_XMIT |
                UART_FCR_R_TRIG_10;
    /* 初始化LCR线路控制寄存器：8位数据 */
    tp->regs[UART_LCR] = UART_LCR_WLEN8;
    /* 保存LCR值到缓存 */
    tp->lcr = UART_LCR_WLEN8;

    /* 初始化接收缓冲区为默认消息 */
    strscpy(tp->rx_buf, DEFAULT_RX_MSG, TTYTEST_RX_BUF_SIZE);

    /* 设置接收数据长度 */
    tp->rx_len = strlen(tp->rx_buf);

    /* 初始化定时器 */
    timer_setup(&tp->rx_timer, ttytest_rx_timer, 0);

    /* 添加UART端口 */
    ret = uart_add_one_port(&ttytest_driver, &tp->port);
    if (ret) {
        pr_err("ttytest: uart_add_one_port failed, ret=%d\n", ret);
        goto err_free_port;
    }

    /* 打印成功信息 */
    pr_info("ttytest: virtual 16550 UART driver loaded\n");
    pr_info("ttytest: device node: /dev/TEST0\n");

    return 0;

/* 错误处理：释放端口内存 */
err_free_port:
    kfree(tp);

/* 错误处理：注销驱动 */
err_unreg_driver:
    uart_unregister_driver(&ttytest_driver);

    return ret;
}

/* 模块退出函数 */
static void __exit ttytest_exit(void)
{
    /* 获取全局端口实例指针 */
    struct ttytest_port *tp = g_ttytest_port;

    /* 检查指针是否为空 */
    if (!tp)
        return;

    /* 移除UART端口 */
    uart_remove_one_port(&ttytest_driver, &tp->port);

    /* 注销UART驱动 */
    uart_unregister_driver(&ttytest_driver);

    /* 释放端口内存 */
    kfree(tp);

    /* 打印卸载信息 */
    pr_info("ttytest: driver unloaded\n");
}

module_init(ttytest_init);
module_exit(ttytest_exit);

MODULE_AUTHOR("embedfire <embedfire@embedfire.com>");
MODULE_DESCRIPTION("tty_subsystem module");
MODULE_LICENSE("GPL v2");