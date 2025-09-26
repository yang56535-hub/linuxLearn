#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/ide.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/of.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>

/***************************************************************
Copyright © ALIENTEK Co., Ltd. 1998-2029. All rights reserved.
文件名 : beep.c
作者 : 杨中凡
版本 : V1.0
描述 : 蜂鸣器驱动程序。
其他 : 无
论坛 : www.openedv.com
日志 : 改版 V1.0 2025/09/26 
***************************************************************/

#define BEEP_CNT    1       /* 设备号个数 */
#define BEEP_NAME   "beep"  /* 名字 */
#define BEEPOFF     0       /* 关蜂鸣器 */
#define BEEPON      1       /* 开蜂鸣器 */

/*
 * @description 	: beep设备结构体
 * @param 			: 无
 * @return			: 无
 */
struct beep_dev{
    dev_t devid;            /* 设备号 */
    struct cdev cdev;       /* cdev */
    struct class *class;    /* 类 */
    struct device *device;  /* 设备 */
    int major;
    int minor;
    struct device_node *nd; /* 设备节点 */
    int beep_gpio;           /* 蜂鸣器所使用的GPIO编号 */
};

struct beep_dev beep; /* beep设备 */

/*
 * @description : 打开设备
 * @param – inode : 传递给驱动的 inode
 * @param - filp : 设备文件， file 结构体有个叫做 private_data 的成员变量
 * 一般在 open 的时候将 private_data 指向设备结构体。
 * @return : 0 成功;其他 失败
 */
static int beep_open(struct inode *inode, struct file *filp)
{
	int ret = 0;
    filp->private_data = &beep; /* 设置私有数据 */
    //printk("beep_open\r\n");
    return ret;
}

/*
 * @description : 从设备读取数据
 * @param - filp : 要打开的设备文件(文件描述符)
 * @param - buf : 返回给用户空间的数据缓冲区
 * @param - cnt : 要读取的数据长度
 * @param - offt : 相对于文件首地址的偏移
 * @retu rn : 读取的字节数，如果为负值，表示读取失败
 */
static ssize_t beep_read(struct file *filp, char __user *buf, size_t cnt, loff_t *offt)
{
    return 0;
}

/*
 * @description : 向设备写数据
 * @param – filp : 设备文件，表示打开的文件描述符
 * @param - buf : 要写给设备写入的数据
 * @param - cnt : 要写入的数据长度
 * @param - offt : 相对于文件首地址的偏移
 * @return : 写入的字节数，如果为负值，表示写入失败
 */
static ssize_t beep_write(struct file *filp, const char __user *buf, size_t cnt, loff_t *offt)
{
    // 从 filp->private_data 中获取在 open 时存入的设备结构体指针
    // struct beep_dev *dev = (struct beep_dev *)filp->private_data;
    // 然后就可以使用 dev->major, dev->minor 等信息

    int ret = 0;
    unsigned char databuf[1];
    unsigned char beepstat;
    struct beep_dev *dev = filp->private_data;

    ret = copy_from_user(databuf, buf, cnt);
    if(ret < 0)
    {
        printk("kernel write failed!\r\n");
        ret = -EFAULT;
    }
    
    beepstat = databuf[0];   /* 获取到应用传递进来的蜂鸣器状态 */

    if(beepstat == BEEPON){   
        gpio_set_value(dev->beep_gpio, 0); /* 打开 蜂鸣器 */
    }else if(beepstat == BEEPOFF){    
        gpio_set_value(dev->beep_gpio, 1); /* 关闭 蜂鸣器 */
    }

    return ret;
}

/*
 * @description : 关闭/释放设备
 * @param – filp : 要关闭的设备文件(文件描述符)
 * @return : 0 成功;其他 失败
 */
static int beep_release(struct inode *inode, struct file *filp)
{
    return 0;
}

 /* 设备操作函数 */
static struct file_operations beep_fops = {
    .owner = THIS_MODULE,
    .open = beep_open,
    .read = beep_read,
    .write = beep_write,
    .release = beep_release,
};

/*
 * @description 	: 驱动入口函数
 * @param 			: 无
 * @return			: 无
 */
static int __init beep_init(void){
    
    int ret = 0;
    const char *str;

    /* 设置 蜂鸣器 所使用的 GPIO */
    /* 1、获取设备节点： beep */
    beep.nd = of_find_node_by_path("/beep");
    if(beep.nd == NULL) {
        printk("beep node not find!\r\n");
        return -EINVAL;
    }

    /* 2.读取 status 属性 */
    ret = of_property_read_string(beep.nd, "status", &str);
    if(ret < 0)
        return -EINVAL;

    if (strcmp(str, "okay"))
        return -EINVAL;

    /* 3、获取 compatible 属性值并进行匹配 */
    ret = of_property_read_string(beep.nd, "compatible", &str);
    if(ret < 0) {
        printk("beep: Failed to get compatible property\n");
        return -EINVAL;
    }

    if (strcmp(str, "alientek,beep")) {
        printk("beep: Compatible match failed\n");
        return -EINVAL;
    }

    /* 4、 获取设备树中的 gpio 属性，得到 beep 所使用的 gpio 编号 */
    beep.beep_gpio = of_get_named_gpio(beep.nd, "beep-gpio", 0);
    if(beep.beep_gpio < 0) {
        printk("can't get beep-gpio");
        return -EINVAL;
    }
    printk("beep-gpio num = %d\r\n", beep.beep_gpio);

    /* 5.向 gpio 子系统申请使用 GPIO */
    ret = gpio_request(beep.beep_gpio, "BEEP-GPIO");
    if (ret) {
        printk(KERN_ERR "beep: Failed to request beep-gpio\n");
        return ret;
    }

    /* 6、设置 PC7 为输出，并且输出高电平，默认关闭 蜂鸣器 */
    ret = gpio_direction_output(beep.beep_gpio, 1);
    if(ret < 0) {
        printk("can't set gpio!\r\n");
        goto free_gpio;
    }

    /* 注册字符设备驱动 */
    /* 1、创建设备号 */
    beep.major = 0;
    if(beep.major){  /* 定义了设备号 */
        beep.devid = MKDEV(beep.major,0);
        ret = register_chrdev_region(beep.devid, BEEP_CNT, BEEP_NAME);

        if(ret < 0){
            pr_err("cannot register %s char driver [ret=%d] \n", BEEP_NAME, BEEP_CNT);
            goto free_gpio;
        }
    }else{
        ret = alloc_chrdev_region(&beep.devid, 0, BEEP_CNT, BEEP_NAME); /* 申请设备号 */
        if(ret < 0){
            pr_err("%s Couldn't alloc_chrdev_region, ret=%d\r\n", BEEP_NAME, ret);
            goto free_gpio;
        }
        beep.major = MAJOR(beep.devid);   /* 获取分配好的主设备号 */
        beep.minor = MINOR(beep.devid);   /* 获取分配好的次设备号 */
    }
    printk("beep major=%d,minor=%d\r\n", beep.major, beep.minor);

    /* 2、初始化cdev */
    beep.cdev.owner = THIS_MODULE;
    cdev_init(&beep.cdev, &beep_fops);

    /* 3、添加一个cdev */
    cdev_add(&beep.cdev, beep.devid, BEEP_CNT);
    if(ret < 0)
        goto del_unregister;

    /* 4、创建类 */
    beep.class = class_create(THIS_MODULE, BEEP_NAME);
    if(IS_ERR(beep.class)){
        goto del_cdev;
    }

    /* 5、创建设备 */
    beep.device = device_create(beep.class, NULL, beep.devid, NULL, BEEP_NAME);
    if(IS_ERR(beep.device))
        goto destroy_class;

    return ret;
destroy_class:
    class_destroy(beep.class);
del_cdev:
    cdev_del(&beep.cdev);
del_unregister:
    unregister_chrdev_region(beep.devid, BEEP_CNT);
free_gpio:
    gpio_free(beep.beep_gpio);
    return -EIO;
}

/*
 * @description 	: 驱动出口函数
 * @param 			: 无
 * @return			: 无
 */
static void __exit beep_exit(void){
    
    /* 驱动卸载时关闭蜂鸣器 */ 
    gpio_set_value(beep.beep_gpio, 1); /* 关闭 蜂鸣器 */

    /* 注销字符设备驱动 */
    cdev_del(&beep.cdev); /* 删除 cdev */
    unregister_chrdev_region(beep.devid, BEEP_CNT);

    /* 删除设备还要用到beep.class，这里先删除设备最后再删除类 */
    device_destroy(beep.class, beep.devid);
    /* 删除类 */
    class_destroy(beep.class);
    /* 释放 GPIO */
    gpio_free(beep.beep_gpio); 
}


/* 注册驱动和卸载驱动 */
module_init(beep_init);   /* 入口 */
module_exit(beep_exit);   /* 出口 */
MODULE_LICENSE("GPL");
MODULE_AUTHOR("ALIENTEK");
MODULE_INFO(intree, "Y");
