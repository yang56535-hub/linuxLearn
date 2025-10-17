/***************************************************************
Copyright © ALIENTEK Co., Ltd. 1998-2029. All rights reserved.
文件名 : ap3216c.c
作者 : 杨中凡
版本 : V1.0
描述 : AP3216C 驱动程序
其他 : 无
论坛 : www.openedv.com
日志 : 初版 V1.0 2021/10/16
 ***************************************************************/
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/ide.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/gpio.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/of_gpio.h>
#include <linux/semaphore.h>
// #include <linux/timer.h>
#include <linux/i2c.h>
#include <asm/mach/map.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include "ap3216creg.h"

#define AP3216C_CNT 1
#define AP3216C_NAME "ap3216c"

/* 设备结构体 */
struct ap3216c_dev {
    struct i2c_client *client;  /* i2c 设备 */
    dev_t devid;                /* 设备号 */
    struct cdev cdev;           /* cdev */
    struct class *class;        /* 类 */
    struct device *device;      /* 设备 */
    struct device_node *nd;     /* 设备节点 */
    unsigned short ir, als, ps; /* 三个光传感器数据 */
    int major;                  /* 主设备号 */
    int minor;                  /* 次设备号 */
};

/*
 * @description : 从 ap3216c 读取多个寄存器数据
 * @param – dev : ap3216c 设备
 * @param – reg : 要读取的寄存器首地址
 * @param – val : 读取到的数据
 * @param – len : 要读取的数据长度
 * @return : 操作结果
 */
static int ap3216c_read_regs(struct ap3216c_dev *dev, u8 reg,
void *val, int len)
{
    int ret;
    struct i2c_msg msg[2];
    struct i2c_client *client = dev->client;

    /* msg[0]，第一条写消息，发送要读取的寄存器首地址 */
    msg[0].addr = client->addr; /* ap3216c 地址 */
    msg[0].flags = 0; /* 标记为发送数据 */
    msg[0].buf = &reg; /* 读取的首地址 */
    msg[0].len = 1; /* reg 长度 */

    /* msg[1]读取数据 */
    msg[1].addr = client->addr; /* ap3216c 地址 */
    msg[1].flags = I2C_M_RD; /* 标记为读取数据 */
    msg[1].buf = val; /* 读取数据缓冲区 */
    msg[1].len = len; /* 要读取的数据长度 */

    ret = i2c_transfer(client->adapter, msg, 2);
    if(ret == 2) {
        ret = 0;
    } else {
        printk("i2c rd failed=%d reg=%06x len=%d\n",ret, reg, len);
        ret = -EREMOTEIO;
    }
    return ret;
}

/*
 * @description: 向 ap3216c 多个寄存器写入数据
 * @param - dev: ap3216c 设备
 * @param - reg: 要写入的寄存器首地址
 * @param - val: 要写入的数据缓冲区
 * @param - len: 要写入的数据长度
 * @return : 操作结果
 */
static s32 ap3216c_write_regs(struct ap3216c_dev *dev, u8 reg, u8 *buf, u8 len)
{
    u8 b[256];
    struct i2c_msg msg;
    struct i2c_client *client = dev->client;
    b[0] = reg; /* 寄存器首地址 */
    memcpy(&b[1],buf,len); /* 将要写入的数据拷贝到数组 b 里面 */

    msg.addr = client->addr; /* ap3216c 地址 */
    msg.flags = 0; /* 标记为写数据 */

    msg.buf = b; /* 要写入的数据缓冲区 */
    msg.len = len + 1; /* 要写入的数据长度，+1是因为还有寄存器首地址要算上 */

    return i2c_transfer(client->adapter, &msg, 1);
}

/*
 * @description: 读取 ap3216c 指定寄存器值，读取一个寄存器
 * @param - dev: ap3216c 设备
 * @param - reg: 要读取的寄存器
 * @return : 读取到的寄存器值
 */
static unsigned char ap3216c_read_reg(struct ap3216c_dev *dev, u8 reg)
{
    u8 data = 0;

    ap3216c_read_regs(dev, reg, &data, 1);
    return data;
}

/*
 * @description: 向 ap3216c 指定寄存器写入指定的值，写一个寄存器
 * @param - dev: ap3216c 设备
 * @param - reg: 要写的寄存器
 * @param - data: 要写入的值
 * @return : 无
 */
static void ap3216c_write_reg(struct ap3216c_dev *dev, u8 reg, u8 data)
{
    u8 buf = 0;
    buf = data;
    ap3216c_write_regs(dev, reg, &buf, 1);
}

/*
 * @description : 读取 AP3216C 的数据，包括 ALS,PS 和 IR, 注意！如果同时
 * :打开 ALS,IR+PS 两次数据读取的时间间隔要大于 112.5ms
 * @param – ir : ir 数据
 * @param - ps : ps 数据
 * @param - ps : als 数据
 * @return : 无。
 */
void ap3216c_readdata(struct ap3216c_dev *dev)
{
    unsigned char i =0;
    unsigned char buf[6];

    /* 循环读取所有传感器数据 */
    for(i = 0; i < 6; i++) {
        buf[i] = ap3216c_read_reg(dev, AP3216C_IRDATALOW + i);
    }

    if(buf[0] & 0X80) /* IR_OF 位为 1,则数据无效 */
        dev->ir = 0;
    else /* 读取 IR 传感器的数据 */
        dev->ir = ((unsigned short)buf[1] << 2) | (buf[0] & 0X03);

    dev->als = ((unsigned short)buf[3] << 8) | buf[2];

    if(buf[4] & 0x40) /* IR_OF 位为 1,则数据无效 */
        dev->ps = 0;
    else /* 读取 PS 传感器的数据 */
        dev->ps = ((unsigned short)(buf[5] & 0X3F) << 4) | (buf[4] &0X0F);
}

/*
 * @description : 打开设备
 * @param – inode : 传递给驱动的 inode
 * @param - filp : 设备文件， file 结构体有个叫做 private_data 的成员变量
 * 一般在 open 的时候将 private_data 指向设备结构体。
 * @return : 0 成功;其他 失败
 */
static int ap3216c_open(struct inode *inode, struct file *filp)
{
    /* 从 file 结构体获取 cdev 指针， 再根据 cdev 获取 ap3216c_dev 首地址 */
    struct cdev *cdev = filp->f_path.dentry->d_inode->i_cdev;
    struct ap3216c_dev *ap3216cdev = container_of(cdev, struct ap3216c_dev, cdev);

    /* 初始化 AP3216C 软复位*/
    ap3216c_write_reg(ap3216cdev, AP3216C_SYSTEMCONG, 0x04);
    mdelay(50);
    /* 使能 ALS+PS+IR */
    ap3216c_write_reg(ap3216cdev, AP3216C_SYSTEMCONG, 0X03);
    return 0;
}

/*
 * @description : 从设备读取数据
 * @param - filp : 要打开的设备文件(文件描述符)
 * @param - buf : 返回给用户空间的数据缓冲区
 * @param - cnt : 要读取的数据长度
 * @param - offt : 相对于文件首地址的偏移
 * @return : 读取的字节数，如果为负值，表示读取失败
 */
static ssize_t ap3216c_read(struct file *filp, char __user *buf,
                            size_t cnt, loff_t *off)
{
    short data[3];
    long err = 0;

    // 1. 通过文件指针找到对应的字符设备结构体
    struct cdev *cdev = filp->f_path.dentry->d_inode->i_cdev;
    // 2. 通过成员字符设备结构体，使用container_of反推其所属结构体的首地址
    struct ap3216c_dev *dev = container_of(cdev, struct ap3216c_dev, cdev);

    ap3216c_readdata(dev);

    data[0] = dev->ir;
    data[1] = dev->als;
    data[2] = dev->ps;
    err = copy_to_user(buf, data, sizeof(data));
    return 0;
}

/*
 * @description : 关闭/释放设备
 * @param - filp : 要关闭的设备文件(文件描述符)
 * @return : 0 成功;其他 失败
 */
static int ap3216c_release(struct inode *inode, struct file *filp)
{
    return 0;
}

/* AP3216C 操作函数 */
static const struct file_operations ap3216c_ops = {
    .owner = THIS_MODULE,
    .open = ap3216c_open,
    .read = ap3216c_read,
    .release = ap3216c_release,
};

/*
 * @description : i2c 驱动的 probe 函数，当驱动与
 * 设备匹配以后此函数就会执行
 * @param – client : i2c 设备
 * @param - id : i2c 设备 ID
 * @return : 0，成功;其他负值,失败
 */
static int ap3216c_probe(struct i2c_client *client, 
                const struct i2c_device_id *id)
{
    int ret;
    struct ap3216c_dev *ap3216cdev;

    /* linux 内核不推荐使用全局变量， 要使用内存的就用 devm_kzalloc 之类的函数去申请空间 */
    ap3216cdev = devm_kzalloc(&client->dev, sizeof(*ap3216cdev), GFP_KERNEL);
    if(!ap3216cdev)
        return -ENOMEM;

    /* 注册字符设备驱动 */
    /* 1、申请设备号 */
    ap3216cdev->major = 0;   /* 设备号由内核分配 */
    if(ap3216cdev->major){   /* 定义了设备号 */
        ap3216cdev->devid = MKDEV(ap3216cdev->major,0);
        ret = register_chrdev_region(ap3216cdev->devid, AP3216C_CNT, AP3216C_NAME);
        if(ret < 0){
            pr_err("cannot register %s char driver[ret=%d]\n",AP3216C_NAME, AP3216C_CNT);
            goto free_gpio;
        }
    }else{              /* 没有给定设备号 */
        ret = alloc_chrdev_region(&ap3216cdev->devid, 0, AP3216C_CNT, AP3216C_NAME);
        ap3216cdev->major = MAJOR(ap3216cdev->devid); /* 获取主设备号 */
        ap3216cdev->minor = MINOR(ap3216cdev->devid); /* 获取次设备号 */
        if(ret < 0) {
        pr_err("%s Couldn't alloc_chrdev_region, ret=%d\r\n", AP3216C_NAME, ret);
            return -ENOMEM;
        }
    }
    printk("ap3216cdev major=%d,minor=%d\r\n",ap3216cdev->major, ap3216cdev->minor);

    /* 2、初始化 cdev */
    ap3216cdev->cdev.owner = THIS_MODULE;
    cdev_init(&ap3216cdev->cdev, &ap3216c_ops);

    /* 3、添加一个 cdev */
    ret = cdev_add(&ap3216cdev->cdev, ap3216cdev->devid, AP3216C_CNT);
    if(ret < 0) {
        goto del_unregister;
    }

    /* 4、创建类 */
    ap3216cdev->class = class_create(THIS_MODULE, AP3216C_NAME);
    if (IS_ERR(ap3216cdev->class)) {
        goto del_cdev;
    }

    /* 5、创建设备 */
    ap3216cdev->device = device_create(ap3216cdev->class, NULL,
                                    ap3216cdev->devid, NULL, AP3216C_NAME);
    if (IS_ERR(ap3216cdev->device)) {
        goto destroy_class;
    }
    /*  由驱动指向设备​​。在自定义结构体中保存I2C设备指针
        让驱动实例能访问具体的I2C设备，以便进行通信（如读写寄存器），
        存储在驱动自定义的结构体 struct ap3216c_dev中。
        提供​​正向​​访问：​​"我有这个设备，我要操作它"​​。 */
    ap3216cdev->client = client;
    /* 由于i2c驱动读写i2c设备寄存器的数据要使用到
    int i2c_transfer(struct i2c_adapter *adap, struct i2c_msg *msgs, int num)
    adap为 I2C 适配器， i2c_client 会保存其对应的 i2c_adapter，因此此处需要
    将 ap3216cdev 变量的地址绑定到 client */

    /*  由设备指向驱动​​。将驱动私有数据关联到I2C设备结构体。
        建立反向查询关系，让驱动在其他函数中能通过client快速找回对应的驱动实例。
        存储在I2C核心层管理的 struct i2c_client中的dev->driver_data字段里
        提供​​反向​​查找：​​"针对这个设备，它的私有数据是什么" 
        比如在ap3216c_remove中使用i2c_get_clientdata得到ap3216c_dev，释放设备*/
    i2c_set_clientdata(client,ap3216cdev);
   
    return 0;

destroy_class:
    class_destroy(ap3216cdev->class);
del_cdev:
    cdev_del(&ap3216cdev->cdev);
del_unregister:
    unregister_chrdev_region(ap3216cdev->devid, AP3216C_CNT);
free_gpio:
    return -ENOMEM;
}



/*
 * @description : i2c 驱动的 remove 函数，移除 i2c 驱动的时候此函数会执行
 * @param - client : i2c 设备
 * @return : 0，成功;其他负值,失败
 */
static int ap3216c_remove(struct i2c_client *client)
{
    struct ap3216c_dev *ap3216cdev = i2c_get_clientdata(client);
    /* 注销字符设备驱动 */
    /* 1、删除 cdev */
    cdev_del(&ap3216cdev->cdev);
    /* 2、注销设备号 */
    unregister_chrdev_region(ap3216cdev->devid, AP3216C_CNT);
    /* 3、注销设备 */
    device_destroy(ap3216cdev->class, ap3216cdev->devid);
    /* 4、注销类 */
    class_destroy(ap3216cdev->class);
    return 0;
}

/* 传统匹配方式 ID 列表 */
static const struct i2c_device_id ap3216c_id[] = {
    {"alientek,ap3216c", 0},
    {}
};

/* 设备树匹配列表 */
static const struct of_device_id ap3216c_of_match[] = {
    { .compatible = "alientek,ap3216c" },
    { /* Sentinel */ }
};

/* i2c 驱动结构体 */
static struct i2c_driver ap3216c_driver = {
    .probe = ap3216c_probe,
    .remove = ap3216c_remove,
    .driver = {
        .owner = THIS_MODULE,
        .name = "ap3216c",
        .of_match_table = ap3216c_of_match,
    },
    .id_table = ap3216c_id,
};

/*
 * @description : 驱动入口函数
 * @param : 无
 * @return : 无
 */
static int __init ap3216c_init(void)
{
    int ret = 0;
    ret = i2c_add_driver(&ap3216c_driver);
    return ret;
}

/*
* @description : 驱动出口函数
* @param : 无
* @return : 无
*/
static void __exit ap3216c_exit(void)
{
    i2c_del_driver(&ap3216c_driver);
}

// 使用宏更简洁
/* module_i2c_driver(ap3216c_driver) */

module_init(ap3216c_init);
module_exit(ap3216c_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("ALIENTEK");
MODULE_INFO(intree, "Y");