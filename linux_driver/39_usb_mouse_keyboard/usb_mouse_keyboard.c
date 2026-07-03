#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/usb.h>
#include <linux/input.h>
#include <linux/slab.h>
#include <linux/version.h>
#include <linux/string.h>
#include <linux/hid.h>
#include <linux/usb/input.h>

/* 设备类型枚举 */
enum hid_device_type {
    DEVICE_TYPE_MOUSE,    /* 鼠标设备类型 */
    DEVICE_TYPE_KEYBOARD  /* 键盘设备类型 */
};

/* 键盘相关常量 */
#define KEYBOARD_REPORT_SIZE 8  /* 标准USB键盘报告长度为8字节 */
#define MAX_KEYS 6              /* 标准USB键盘支持最多6键同时按下 */

/* 鼠标相关常量 */
#define MOUSE_REPORT_SIZE 8     /* 标准USB鼠标报告最大长度为8字节 */

/* 驱动私有数据结构体 */
struct usb_hid_device {
    struct usb_device *udev;          /* 指向USB设备对象的指针*/
    struct input_dev *input;          /* 指向输入设备对象的指针 */
    struct urb *irq_urb;              /* 指向中断URB的指针，用于接收设备的中断传输数据 */
    unsigned char *data;              /* DMA缓冲区的虚拟地址，存储设备发送的数据 */
    dma_addr_t data_dma;              /* DMA缓冲区的物理地址，供USB控制器直接访问 */
    char name[64];                    /* 设备名称 */
    char phys[64];                    /* 设备物理路径 */
    enum hid_device_type type;        /* 设备类型，标记是鼠标还是键盘 */
    int interface_num;                /* 当前设备对应的USB接口号 */
    
    /* 键盘专用 */
    unsigned char old_keys[MAX_KEYS]; /* 保存上一次的按键状态，用于检测按键释放 */
    unsigned char old_modifiers;      /* 保存上一次的修饰键(Ctrl/Shift等)状态 */
    
    /* 鼠标专用 */
    int has_extra_buttons;            /* 标记鼠标是否支持额外侧键 */
};

/************************** 键盘功能相关代码 **************************/

/* USB HID键盘按键码到Linux输入子系统键码的转换表
 * 索引: USB HID标准按键码(0x00-0x73)
 * 值: Linux输入子系统定义的键码(KEY_*系列宏)
 */
static const unsigned char usb_keycode_map[256] = {
	KEY_RESERVED, KEY_RESERVED, KEY_RESERVED, KEY_RESERVED, KEY_A, KEY_B,
	KEY_C, KEY_D, KEY_E, KEY_F, KEY_G, KEY_H, KEY_I, KEY_J, KEY_K, KEY_L,
	KEY_M, KEY_N, KEY_O, KEY_P, KEY_Q, KEY_R, KEY_S, KEY_T, KEY_U, KEY_V,
	KEY_W, KEY_X, KEY_Y, KEY_Z, KEY_1, KEY_2, KEY_3, KEY_4, KEY_5, KEY_6,
	KEY_7, KEY_8, KEY_9, KEY_0, KEY_ENTER, KEY_ESC, KEY_BACKSPACE,
	KEY_TAB, KEY_SPACE, KEY_MINUS, KEY_EQUAL, KEY_LEFTBRACE,
	KEY_RIGHTBRACE, KEY_BACKSLASH, KEY_BACKSLASH, KEY_SEMICOLON,
	KEY_APOSTROPHE, KEY_GRAVE, KEY_COMMA, KEY_DOT, KEY_SLASH,
	KEY_CAPSLOCK, KEY_F1, KEY_F2, KEY_F3, KEY_F4, KEY_F5, KEY_F6,
	KEY_F7, KEY_F8, KEY_F9, KEY_F10, KEY_F11, KEY_F12, KEY_SYSRQ,
	KEY_SCROLLLOCK, KEY_PAUSE, KEY_INSERT, KEY_HOME, KEY_PAGEUP,
	KEY_DELETE, KEY_END, KEY_PAGEDOWN, KEY_RIGHT, KEY_LEFT, KEY_DOWN,
	KEY_UP, KEY_NUMLOCK, KEY_KPSLASH, KEY_KPASTERISK, KEY_KPMINUS,
	KEY_KPPLUS, KEY_KPENTER, KEY_KP1, KEY_KP2, KEY_KP3, KEY_KP4, KEY_KP5,
	KEY_KP6, KEY_KP7, KEY_KP8, KEY_KP9, KEY_KP0, KEY_KPDOT, KEY_102ND,
	KEY_COMPOSE, KEY_POWER, KEY_KPEQUAL, KEY_F13, KEY_F14, KEY_F15,
	KEY_F16, KEY_F17, KEY_F18, KEY_F19, KEY_F20, KEY_F21, KEY_F22,
	KEY_F23, KEY_F24, KEY_OPEN, KEY_HELP, KEY_PROPS, KEY_FRONT, KEY_STOP,
	KEY_AGAIN, KEY_UNDO, KEY_CUT, KEY_COPY, KEY_PASTE, KEY_FIND, KEY_MUTE,
	KEY_VOLUMEUP, KEY_VOLUMEDOWN, KEY_RESERVED, KEY_RESERVED, KEY_RESERVED,
	KEY_KPCOMMA, KEY_RESERVED, KEY_RO, KEY_KATAKANAHIRAGANA , KEY_YEN,
	KEY_HENKAN, KEY_MUHENKAN, KEY_KPJPCOMMA, KEY_RESERVED, KEY_RESERVED,
	KEY_RESERVED, KEY_HANGEUL, KEY_HANJA, KEY_KATAKANA, KEY_HIRAGANA,
	KEY_ZENKAKUHANKAKU, KEY_RESERVED, KEY_RESERVED, KEY_RESERVED,
	KEY_RESERVED, KEY_RESERVED, KEY_RESERVED, KEY_RESERVED, KEY_RESERVED,
	KEY_RESERVED, KEY_RESERVED, KEY_RESERVED, KEY_RESERVED, KEY_RESERVED,
	KEY_RESERVED, KEY_RESERVED, KEY_RESERVED, KEY_RESERVED, KEY_RESERVED,
	KEY_RESERVED, KEY_RESERVED, KEY_RESERVED, KEY_RESERVED, KEY_RESERVED,
	KEY_RESERVED, KEY_RESERVED, KEY_RESERVED, KEY_RESERVED, KEY_RESERVED,
	KEY_RESERVED, KEY_RESERVED, KEY_RESERVED, KEY_RESERVED, KEY_RESERVED,
	KEY_RESERVED, KEY_RESERVED, KEY_RESERVED, KEY_RESERVED, KEY_RESERVED,
	KEY_RESERVED, KEY_RESERVED, KEY_RESERVED, KEY_RESERVED, KEY_RESERVED,
	KEY_RESERVED, KEY_RESERVED, KEY_RESERVED, KEY_RESERVED, KEY_RESERVED,
	KEY_RESERVED, KEY_RESERVED, KEY_RESERVED, KEY_RESERVED, KEY_RESERVED,
	KEY_RESERVED, KEY_RESERVED, KEY_RESERVED, KEY_RESERVED, KEY_RESERVED,
	KEY_RESERVED, KEY_RESERVED, KEY_RESERVED, KEY_RESERVED, KEY_RESERVED,
	KEY_RESERVED, KEY_RESERVED, KEY_RESERVED, KEY_RESERVED, KEY_RESERVED,
	KEY_RESERVED, KEY_RESERVED, KEY_RESERVED, KEY_RESERVED, KEY_RESERVED,
	KEY_RESERVED, KEY_RESERVED, KEY_LEFTCTRL, KEY_LEFTSHIFT, KEY_LEFTALT,
	KEY_LEFTMETA, KEY_RIGHTCTRL, KEY_RIGHTSHIFT, KEY_RIGHTALT,
	KEY_RIGHTMETA, KEY_PLAYPAUSE, KEY_STOPCD, KEY_PREVIOUSSONG,
	KEY_NEXTSONG, KEY_EJECTCD, KEY_VOLUMEUP, KEY_VOLUMEDOWN, KEY_MUTE,
	KEY_WWW, KEY_BACK, KEY_FORWARD, KEY_STOP, KEY_FIND, KEY_SCROLLUP,
	KEY_SCROLLDOWN, KEY_EDIT, KEY_SLEEP, KEY_SCREENLOCK, KEY_REFRESH,
	KEY_CALC, KEY_RESERVED, KEY_RESERVED, KEY_RESERVED, KEY_RESERVED
};

/* 修饰键映射表，将USB报告中的修饰键位映射到Linux输入子系统键码
 * 索引: USB报告第一个字节的位位置(0-7)
 * 值: Linux输入子系统定义的键码(KEY_*系列宏)
 */
static const unsigned char modifier_map[8] = {
    KEY_LEFTCTRL, KEY_LEFTSHIFT, KEY_LEFTALT, KEY_LEFTMETA,
    KEY_RIGHTCTRL, KEY_RIGHTSHIFT, KEY_RIGHTALT, KEY_RIGHTMETA
};

/* 键盘中断处理函数 */
static void usb_kbd_irq(struct urb *urb)
{
    /* 从URB的上下文获取自定义的HID设备结构体 */
    struct usb_hid_device *kbd = urb->context;
    /* 获取存储键盘数据的DMA缓冲区指针 */
    unsigned char *data = kbd->data;
    /* 获取输入设备指针 */
    struct input_dev *dev = kbd->input;
    /* 定义变量存储URB重新提交的返回状态 */
    int status;
    /* 循环计数器变量 */
    int i, j;
    /* 存储当前报告中的修饰键状态 */
    unsigned char modifiers;
    /* 标记按键是否仍然处于按下状态 */
    int key_pressed;

    /* 检查URB传输的完成状态 */
    switch (urb->status) {
    case 0: /* 状态0表示传输成功完成，继续处理数据 */
        break;
    case -ECONNRESET:   /* 连接被重置，通常是设备被拔出 */
    case -ENOENT:       /* URB被主动取消 */
    case -ESHUTDOWN:    /* 设备正在关闭 */
        return;         /* 这些情况表示设备已经断开，不需要重新提交URB */
    default:
        /* 其他所有非致命错误，重新提交URB继续接收数据 */
        goto resubmit;
    }

    /* 标准USB键盘报告格式(8字节)：
     * Byte 0: 修饰键状态(bit0=左Ctrl, bit1=左Shift, bit2=左Alt, bit3=左Win
     *                    bit4=右Ctrl, bit5=右Shift, bit6=右Alt, bit7=右Win)
     * Byte 1: 保留字节，始终为0
     * Byte 2-7: 当前按下的按键码(最多6个，0表示无按键)
     */

    /* 从报告第一个字节获取当前修饰键状态 */
    modifiers = data[0];

    /* 处理修饰键的状态变化 */
    for (i = 0; i < 8; i++) {
        /* 比较当前修饰键位与上一次保存的状态是否不同 */
        if ((modifiers & (1 << i)) != (kbd->old_modifiers & (1 << i))) {
            /* 上报修饰键的按下(1)或释放(0)事件 */
            input_report_key(dev, modifier_map[i], (modifiers & (1 << i)) ? 1 : 0);
        }
    }
    /* 保存当前修饰键状态，用于下一次比较 */
    kbd->old_modifiers = modifiers;

    /* 处理按键释放事件：检查上一次按下的按键是否仍然按下 */
    for (i = 0; i < MAX_KEYS; i++) {
        /* 跳过0值，表示该位置没有按键 */
        if (kbd->old_keys[i] == 0)
            continue;

        /* 初始化按键按下标记为假 */
        key_pressed = 0;
        /* 在当前报告中查找该按键是否仍然存在 */
        for (j = 2; j < KEYBOARD_REPORT_SIZE; j++) {
            if (data[j] == kbd->old_keys[i]) {
                /* 找到该按键，标记为仍然按下 */
                key_pressed = 1;
                break;
            }
        }

        /* 如果按键不再按下，上报释放事件 */
        if (!key_pressed) {
            /* 将USB按键码转换为Linux输入子系统键码 */
            unsigned int keycode = usb_keycode_map[kbd->old_keys[i]];
            /* 如果转换有效，上报按键释放事件(0表示释放) */
            if (keycode != 0)
                input_report_key(dev, keycode, 0);
        }
    }

    /* 处理按键按下事件：检查当前报告中的按键是否是新按下的 */
    for (i = 2; i < KEYBOARD_REPORT_SIZE; i++) {
        /* 跳过0值，表示该位置没有按键 */
        if (data[i] == 0)
            continue;

        /* 初始化按键按下标记为假 */
        key_pressed = 0;
        /* 在上一次保存的按键状态中查找该按键 */
        for (j = 0; j < MAX_KEYS; j++) {
            if (kbd->old_keys[j] == data[i]) {
                /* 找到该按键，说明已经按下，不需要重复上报 */
                key_pressed = 1;
                break;
            }
        }

        /* 如果是新按下的按键，上报按下事件 */
        if (!key_pressed) {
            /* 将USB按键码转换为Linux输入子系统键码 */
            unsigned int keycode = usb_keycode_map[data[i]];
            /* 如果转换有效，上报按键按下事件(1表示按下) */
            if (keycode != 0)
                input_report_key(dev, keycode, 1);
        }
    }

    /* 保存当前按键状态，用于下一次比较 */
    memcpy(kbd->old_keys, &data[2], MAX_KEYS);

    /* 同步输入事件，通知用户空间所有事件已上报完毕 */
    input_sync(dev);

resubmit:
    /* 重新提交中断URB，以继续接收下一次键盘数据 */
    status = usb_submit_urb(urb, GFP_ATOMIC);
    if (status)
        dev_err(&kbd->udev->dev,
                "Unable to resubmit keyboard URB, error code: %d\n", status);
}

/************************** 鼠标功能相关代码 **************************/

/* 鼠标中断处理函数 */
static void usb_mouse_irq(struct urb *urb)
{
    /* 从URB的上下文获取自定义的HID设备结构体 */
    struct usb_hid_device *mouse = urb->context;
    /* 获取存储鼠标数据的DMA缓冲区指针 */
    signed char *data = mouse->data;
    /* 获取输入设备指针 */
    struct input_dev *dev = mouse->input;
    /* 定义变量存储URB重新提交的返回状态 */
    int status;

    /* 检查URB传输的完成状态 */
    switch (urb->status) {
    case 0:
        /* 状态0表示传输成功完成，继续处理数据 */
        break;
    case -ECONNRESET:   /* 连接被重置，通常是设备被拔出 */
    case -ENOENT:       /* URB被主动取消 */
    case -ESHUTDOWN:    /* 设备正在关闭 */
        return;         /* 这些情况表示设备已经断开，不需要重新提交URB */
    default:
        /* 其他所有非致命错误，重新提交URB继续接收数据 */
        goto resubmit;
    }

    /* 标准USB鼠标报告格式(4字节，支持滚轮)：
     * Byte 0: 按钮状态(bit0=左键, bit1=右键, bit2=中键, bit3=侧键1, bit4=侧键2)
     * Byte 1: X轴相对位移(-127 ~ +127，正值向右，负值向左)
     * Byte 2: Y轴相对位移(-127 ~ +127，正值向下，负值向上)
     * Byte 3: 滚轮位移(-127 ~ +127，正值向上滚动，负值向下滚动)
     */

    /* 上报鼠标左键状态，非0表示按下，0表示释放 */
    input_report_key(dev, BTN_LEFT,   data[0] & 0x01);
    /* 上报鼠标右键状态 */
    input_report_key(dev, BTN_RIGHT,  data[0] & 0x02);
    /* 上报鼠标中键状态 */
    input_report_key(dev, BTN_MIDDLE, data[0] & 0x04);
    
    /* 上报鼠标侧键1状态 */
    input_report_key(dev, BTN_SIDE,   data[0] & 0x08);
    /* 上报鼠标侧键2状态 */
    input_report_key(dev, BTN_EXTRA,  data[0] & 0x10);

    /* 上报X轴方向的相对移动量 */
    input_report_rel(dev, REL_X, data[1]);
    /* 上报Y轴方向的相对移动量 */
    input_report_rel(dev, REL_Y, data[2]);

    /* 上报滚轮移动量 */
    input_report_rel(dev, REL_WHEEL, data[3]);

    /* 同步输入事件，通知用户空间所有事件已上报完毕 */
    input_sync(dev);

resubmit:
    /* 重新提交中断URB，以继续接收下一次鼠标数据 */
    status = usb_submit_urb(urb, GFP_ATOMIC);
    if (status)
        dev_err(&mouse->udev->dev,
                "Unable to resubmit mouse URB, error code: %d\n", status);
}

/* 兼容不同内核版本的usb_maxpacket函数 */
static inline u16 usb_hid_maxpacket(struct usb_device *udev, int pipe)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 247)
    /* 新内核版本使用两个参数的函数，自动从pipe中判断方向 */
    return usb_maxpacket(udev, pipe);
#else
    /* 旧内核版本使用三个参数的函数，第三个参数表示是否为输出管道 */
    return usb_maxpacket(udev, pipe, usb_pipeout(pipe));
#endif
}

/* 设备探测函数 */
static int usb_hid_probe(struct usb_interface *intf,
                         const struct usb_device_id *id)
{
    /* 从接口获取对应的USB设备指针 */
    struct usb_device *udev = interface_to_usbdev(intf);
    /* 指向当前激活的接口设置描述符 */
    struct usb_host_interface *iface_desc;
    /* 指向端点描述符的指针 */
    struct usb_endpoint_descriptor *endpoint;
    /* 指向自定义的HID设备结构体 */
    struct usb_hid_device *hid_dev;
    /* 指向输入设备结构体 */
    struct input_dev *input_dev;
    /* USB中断接收管道号 */
    int pipe;
    /* 端点支持的最大数据包大小 */
    int maxp;
    /* 错误码变量 */
    int error;
    /* 循环计数器变量 */
    int i;

    /* 获取当前接口的激活设置描述符 */
    iface_desc = intf->cur_altsetting;

    /* 基础协议检查：验证是否为HID类引导子类设备 */
    if (iface_desc->desc.bInterfaceClass != USB_INTERFACE_CLASS_HID ||
        iface_desc->desc.bInterfaceSubClass != USB_INTERFACE_SUBCLASS_BOOT) {
        return -ENODEV;
    }

    /* 检查接口端点数量：标准HID引导设备只有一个中断输入端点 */
    if (iface_desc->desc.bNumEndpoints != 1)
        return -ENODEV;

    /* 获取第一个也是唯一一个端点的描述符 */
    endpoint = &iface_desc->endpoint[0].desc;
    /* 检查该端点是否为中断输入端点 */
    if (!usb_endpoint_is_int_in(endpoint))
        return -ENODEV;

    /* 分配通用HID设备结构体的内存，并初始化为0 */
    hid_dev = kzalloc(sizeof(struct usb_hid_device), GFP_KERNEL);
    if (!hid_dev)
        return -ENOMEM;

    /* 分配输入设备结构体 */
    input_dev = input_allocate_device();
    if (!input_dev)
        goto fail_free_hid_dev;

    /* 设置USB设备指针 */
    hid_dev->udev = udev;
    /* 设置输入设备指针 */
    hid_dev->input = input_dev;
    /* 保存当前接口号，用于后续控制传输 */
    hid_dev->interface_num = iface_desc->desc.bInterfaceNumber;

    /* 根据接口协议类型区分是鼠标还是键盘设备 */
    if (iface_desc->desc.bInterfaceProtocol == USB_INTERFACE_PROTOCOL_MOUSE) {
        /* 设置设备类型为鼠标 */
        hid_dev->type = DEVICE_TYPE_MOUSE;
        /* 格式化鼠标设备名称 */
        snprintf(hid_dev->name, sizeof(hid_dev->name), "USB Mouse");
        
        /* 默认支持额外侧键 */
        hid_dev->has_extra_buttons = 1;
        
        /* 设置鼠标输入设备支持的事件类型：按键事件和相对坐标事件 */
        input_dev->evbit[0] = BIT_MASK(EV_KEY) | BIT_MASK(EV_REL);
        /* 设置鼠标支持的按键：左键、右键、中键、侧键1、侧键2 */
        input_dev->keybit[BIT_WORD(BTN_MOUSE)] =
            BIT_MASK(BTN_LEFT) | BIT_MASK(BTN_RIGHT) | BIT_MASK(BTN_MIDDLE) |
            BIT_MASK(BTN_SIDE) | BIT_MASK(BTN_EXTRA);
        /* 设置鼠标支持的相对坐标：X轴和Y轴 */
        input_dev->relbit[0] = BIT_MASK(REL_X) | BIT_MASK(REL_Y);
        
        /* 设置鼠标滚轮相对坐标 */
        input_dev->relbit[0] |= BIT_MASK(REL_WHEEL);
            
    } else if (iface_desc->desc.bInterfaceProtocol == USB_INTERFACE_PROTOCOL_KEYBOARD) {
        /* 设置设备类型为键盘 */
        hid_dev->type = DEVICE_TYPE_KEYBOARD;
        /* 格式化键盘设备名称 */
        snprintf(hid_dev->name, sizeof(hid_dev->name), "USB Keyboard");
        
        /* 初始化按键状态数组为全0 */
        memset(hid_dev->old_keys, 0, MAX_KEYS);
        /* 初始化修饰键状态为0 */
        hid_dev->old_modifiers = 0;
        
        /* 设置键盘输入设备支持的事件类型：按键事件 */
        input_dev->evbit[0] = BIT_MASK(EV_KEY);
        
        /* 设置键盘支持的所有按键 */
        for (i = 0; i < 256; i++) {
            /* 如果转换表中有对应的键码，设置该按键位 */
            if (usb_keycode_map[i] != 0)
                set_bit(usb_keycode_map[i], input_dev->keybit);
        }
        
    } else {
        /* 不支持的设备类型 */
        error = -ENODEV;
        goto fail_free_input_dev;
    }

    /* 分配DMA一致性缓冲区，用于接收设备数据 */
    hid_dev->data = usb_alloc_coherent(udev, 8, GFP_KERNEL, &hid_dev->data_dma);
    if (!hid_dev->data)
        goto fail_free_input_dev;

    /* 分配一个URB结构体，用于中断传输 */
    hid_dev->irq_urb = usb_alloc_urb(0, GFP_KERNEL);
    if (!hid_dev->irq_urb)
        goto fail_free_buffer;

    /* 创建USB中断接收管道 */
    pipe = usb_rcvintpipe(udev, endpoint->bEndpointAddress);
    /* 获取该端点支持的最大数据包大小 */
    maxp = usb_hid_maxpacket(udev, pipe);
    /* 限制最大数据包大小为8字节，只需要前8字节 */
    if (maxp > 8)
        maxp = 8;

    /* 初始化中断URB，根据设备类型设置不同的回调函数 */
    if (hid_dev->type == DEVICE_TYPE_MOUSE) {
        /* 鼠标设备使用鼠标中断处理函数 */
        usb_fill_int_urb(hid_dev->irq_urb, udev, pipe,
                         hid_dev->data, maxp,
                         usb_mouse_irq, hid_dev,
                         endpoint->bInterval);
    } else {
        /* 键盘设备使用键盘中断处理函数 */
        usb_fill_int_urb(hid_dev->irq_urb, udev, pipe,
                         hid_dev->data, maxp,
                         usb_kbd_irq, hid_dev,
                         endpoint->bInterval);
    }
    
    /* 设置URB的DMA物理地址 */
    hid_dev->irq_urb->transfer_dma = hid_dev->data_dma;
    /* 设置URB标志，告诉USB核心不要为传输数据重新映射DMA地址 */
    hid_dev->irq_urb->transfer_flags |= URB_NO_TRANSFER_DMA_MAP;

    /* 格式化设备物理路径 */
    snprintf(hid_dev->phys, sizeof(hid_dev->phys),
             "usb-%s-%s/input0", udev->bus->bus_name,
             udev->devpath);

    /* 设置输入设备的名称 */
    input_dev->name = hid_dev->name;
    /* 设置输入设备的物理路径 */
    input_dev->phys = hid_dev->phys;
    /* 从USB设备信息填充输入设备的ID信息 */
    usb_to_input_id(udev, &input_dev->id);
    /* 设置输入设备的父设备为USB接口设备 */
    input_dev->dev.parent = &intf->dev;

    /* 注册输入设备到内核输入子系统 */
    error = input_register_device(input_dev);
    if (error)
        goto fail_free_urb;

    /* 将HID设备结构体指针保存到USB接口的私有数据中 */
    usb_set_intfdata(intf, hid_dev);

    /* 提交中断URB，开始接收设备数据 */
    error = usb_submit_urb(hid_dev->irq_urb, GFP_KERNEL);
    if (error) {
        dev_err(&intf->dev, "Unable to submit URB, error code: %d\n", error);
        goto fail_unregister_input;
    }

    /* 打印设备连接成功信息，包含设备名称和接口号 */
    dev_info(&intf->dev, "%sConnected (Interface Number: %d)\n", hid_dev->name, hid_dev->interface_num);

    return 0;


fail_unregister_input:
    /* 从输入子系统注销输入设备 */
    input_unregister_device(input_dev);

fail_free_urb:
    /* 释放分配的URB结构体 */
    usb_free_urb(hid_dev->irq_urb);

fail_free_buffer:
    /* 释放分配的DMA一致性缓冲区 */
    usb_free_coherent(udev, 8, hid_dev->data, hid_dev->data_dma);

fail_free_input_dev:
    /* 释放分配的输入设备结构体 */
    input_free_device(input_dev);

fail_free_hid_dev:
    /* 释放分配的自定义HID设备结构体 */
    kfree(hid_dev);

    return error;
}

/* 设备断开函数 */
static void usb_hid_disconnect(struct usb_interface *intf)
{
    /* 从USB接口的私有数据中获取HID设备结构体 */
    struct usb_hid_device *hid_dev = usb_get_intfdata(intf);

    /* 将USB接口的私有数据指针置空 */
    usb_set_intfdata(intf, NULL);

    /* 检查HID设备结构体是否存在 */
    if (hid_dev) {
        /* 停止并杀死任何正在进行的URB传输 */
        usb_kill_urb(hid_dev->irq_urb);

        /* 释放URB结构体 */
        usb_free_urb(hid_dev->irq_urb);

        /* 释放DMA一致性缓冲区 */
        usb_free_coherent(hid_dev->udev, 8, hid_dev->data, hid_dev->data_dma);

        /* 从输入子系统注销并释放输入设备 */
        input_unregister_device(hid_dev->input);

        /* 释放自定义HID设备结构体 */
        kfree(hid_dev);
    }

    /* 打印设备断开信息 */
    dev_info(&intf->dev, "USB HID device has been disconnected\n");
}

/* USB设备ID表 */
static const struct usb_device_id usb_mouse_keyboard_id_table[] = {
    /* 匹配所有符合HID类、引导子类、鼠标协议的USB接口 */
    { USB_INTERFACE_INFO(USB_INTERFACE_CLASS_HID,
                         USB_INTERFACE_SUBCLASS_BOOT,
                         USB_INTERFACE_PROTOCOL_MOUSE) },
    /* 匹配所有符合HID类、引导子类、键盘协议的USB接口 */
    { USB_INTERFACE_INFO(USB_INTERFACE_CLASS_HID,
                         USB_INTERFACE_SUBCLASS_BOOT,
                         USB_INTERFACE_PROTOCOL_KEYBOARD) },
    { }
};
/* 导出设备ID表，使内核热插拔系统知道该驱动支持哪些设备 */
MODULE_DEVICE_TABLE(usb, usb_mouse_keyboard_id_table);

/* USB驱动结构体 */
static struct usb_driver usb_mouse_keyboard_driver = {
    .name       = "usb-mouse-keyboard",
    .id_table   = usb_mouse_keyboard_id_table,
    .probe      = usb_hid_probe,        /* 探测函数指针，设备插入时调用 */
    .disconnect = usb_hid_disconnect,   /* 断开函数指针，设备拔出时调用 */
};

/* 模块初始化函数 */
static int __init usb_mouse_keyboard_init(void)
{
    int result;

    /* 向USB子系统注册USB驱动 */
    result = usb_register(&usb_mouse_keyboard_driver);
    if (result)
        pr_err("Failed to register USB driver, error code: %d\n", result);

    return result;
}

/* 模块退出函数 */
static void __exit usb_mouse_keyboard_exit(void)
{
    /* 从USB子系统注销USB驱动 */
    usb_deregister(&usb_mouse_keyboard_driver);

    /* 打印模块卸载信息 */
    pr_info("USB mouse and keyboard driver has been uninstalled\n");
}

module_init(usb_mouse_keyboard_init);
module_exit(usb_mouse_keyboard_exit);

MODULE_AUTHOR("embedfire <embedfire@embedfire.com>");
MODULE_DESCRIPTION("usb_mouse_keyboard module");
MODULE_LICENSE("GPL v2");