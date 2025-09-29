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
文件名 : atomic.c
作者 : 杨中凡
版本 : V1.0
描述 : 原子操作实验，使用原子变量来实现对实现设备的互斥访问。
其他 : 无
论坛 : www.openedv.com
日志 : 改版 V1.0 2025/09/27 
***************************************************************/

#define GPIOLED_CNT 1            /* 设备号个数 */
#define GPIOLED_NAME "gpioled"    /* 名字 */
#define LEDOFF 0 /* 关灯 */
#define LEDON 1 /* 开灯 */

/*
 * @description 	: gpioled设备结构体
 * @param 			: 无
 * @return			: 无
 */
struct gpioled_dev{
    dev_t devid;            /* 设备号 */
    struct cdev cdev;       /* cdev */
    struct class *class;    /* 类 */
    struct device *device;  /* 设备 */
    int major;              /* 主设备号 */
    int minor;
    struct device_node *nd; /* 设备节点 */
    int led_gpio;           /* led所使用的GPIO编号 */
    atomic_t lock;          /* 原子变量 */
};

struct gpioled_dev gpioled; /* led设备 */

/*
 * @description : 打开设备
 * @param – inode : 传递给驱动的 inode
 * @param - filp : 设备文件， file 结构体有个叫做 private_data 的成员变量
 * 一般在 open 的时候将 private_data 指向设备结构体。
 * @return : 0 成功;其他 失败
 */
static int led_open(struct inode *inode, struct file *filp)
{
	int ret = 0;
    /* 通过判断原子变量的值来检查LED有没有被别的应用使用 */
    if(!atomic_dec_and_test(&gpioled.lock)){    /* 从 lock 减 1，如果结果为 0 就返回真，否则返回假 */
        atomic_inc(&gpioled.lock);  /* 小于 0 的话就加 1,使其原子变量等于 0 */
        return -EBUSY;              /* LED 被使用，返回忙 */
    }

    filp->private_data = &gpioled; /* 设置私有数据 */
    //printk("led_open\r\n");
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
static ssize_t led_read(struct file *filp, char __user *buf, size_t cnt, loff_t *offt)
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
static ssize_t led_write(struct file *filp, const char __user *buf, size_t cnt, loff_t *offt)
{
    // 从 filp->private_data 中获取在 open 时存入的设备结构体指针
    // struct gpioled_dev *dev = (struct gpioled_dev *)filp->private_data;
    // 然后就可以使用 dev->major, dev->minor 等信息

    int ret = 0;
    unsigned char databuf[1];
    unsigned char ledstat;
    struct gpioled_dev *dev = filp->private_data;

    ret = copy_from_user(databuf, buf, cnt);
    if(ret < 0)
    {
        printk("kernel write failed!\r\n");
        ret = -EFAULT;
    }
    
    ledstat = databuf[0];   /* 获取到应用传递进来的开关灯状态 */

    if(ledstat == LEDON){   
        gpio_set_value(dev->led_gpio, 0); /* 打开 LED 灯 */
    }else if(ledstat == LEDOFF){    
        gpio_set_value(dev->led_gpio, 1); /* 关闭 LED 灯 */
    }

    return ret;
}

/*
 * @description : 关闭/释放设备
 * @param – filp : 要关闭的设备文件(文件描述符)
 * @return : 0 成功;其他 失败
 */
static int led_release(struct inode *inode, struct file *filp)
{
    struct gpioled_dev *dev = filp->private_data;
    /* 关闭驱动文件的时候释放原子变量 */
    atomic_inc(&dev->lock);

    return 0;
}

 /* 设备操作函数 */
static struct file_operations gpioled_fops = {
    .owner = THIS_MODULE,
    .open = led_open,
    .read = led_read,
    .write = led_write,
    .release = led_release,
};

/*
 * @description 	: 驱动入口函数
 * @param 			: 无
 * @return			: 无
 */
static int __init led_init(void){
    
    int ret = 0;
    const char *str;

    /* 1、初始化原子变量 */
    gpioled.lock = (atomic_t)ATOMIC_INIT(0);  //疑问：为什么要强转,不强转编译不过

    /* 2、原子变量初始值为 1 */
    atomic_set(&gpioled.lock, 1);

    /* 设置 LED 所使用的 GPIO */
    /* 1、获取设备节点： gpioled */
    gpioled.nd = of_find_node_by_path("/gpioled");
    if(gpioled.nd == NULL) {
        printk("gpioled node not find!\r\n");
        return -EINVAL;
    }

    /* 2.读取 status 属性 */
    ret = of_property_read_string(gpioled.nd, "status", &str);
    if(ret < 0)
        return -EINVAL;

    if (strcmp(str, "okay"))
        return -EINVAL;

    /* 3、获取 compatible 属性值并进行匹配 */
    ret = of_property_read_string(gpioled.nd, "compatible", &str);
    if(ret < 0) {
        printk("gpioled: Failed to get compatible property\n");
        return -EINVAL;
    }

    if (strcmp(str, "alientek,led")) {
        printk("gpioled: Compatible match failed\n");
        return -EINVAL;
    }

    /* 4、 获取设备树中的 gpio 属性，得到 LED 所使用的 LED 编号 */
    gpioled.led_gpio = of_get_named_gpio(gpioled.nd, "led-gpio", 0);
    if(gpioled.led_gpio < 0) {
        printk("can't get led-gpio");
        return -EINVAL;
    }
    printk("led-gpio num = %d\r\n", gpioled.led_gpio);

    /* 5.向 gpio 子系统申请使用 GPIO */
    ret = gpio_request(gpioled.led_gpio, "LED-GPIO");
    if (ret) {
        printk(KERN_ERR "gpioled: Failed to request led-gpio\n");
        return ret;
    }

    /* 6、设置 PI0 为输出，并且输出高电平，默认关闭 LED 灯 */
    ret = gpio_direction_output(gpioled.led_gpio, 1);
    if(ret < 0) {
        printk("can't set gpio!\r\n");
        goto free_gpio;
    }

    /* 注册字符设备驱动 */
    /* 1、创建设备号 */
    gpioled.major = 0;
    if(gpioled.major){  /* 定义了设备号 */
        gpioled.devid = MKDEV(gpioled.major,0);
        ret = register_chrdev_region(gpioled.devid, GPIOLED_CNT,GPIOLED_NAME);

        if(ret < 0){
            pr_err("cannot register %s char driver [ret=%d] \n", GPIOLED_NAME, GPIOLED_CNT);
            goto free_gpio;
        }
    }else{
        ret = alloc_chrdev_region(&gpioled.devid, 0, GPIOLED_CNT, GPIOLED_NAME); /* 申请设备号 */
        if(ret < 0){
            pr_err("%s Couldn't alloc_chrdev_region, ret=%d\r\n", GPIOLED_NAME, ret);
            goto free_gpio;
        }
        gpioled.major = MAJOR(gpioled.devid);   /* 获取分配好的主设备号 */
        gpioled.minor = MINOR(gpioled.devid);   /* 获取分配好的次设备号 */
    }
    printk("gpioled major=%d,minor=%d\r\n", gpioled.major, gpioled.minor);

    /* 2、初始化cdev */
    gpioled.cdev.owner = THIS_MODULE;
    cdev_init(&gpioled.cdev, &gpioled_fops);

    /* 3、添加一个cdev */
    cdev_add(&gpioled.cdev, gpioled.devid, GPIOLED_CNT);
    if(ret < 0)
        goto del_unregister;

    /* 4、创建类 */
    gpioled.class = class_create(THIS_MODULE, GPIOLED_NAME);
    if(IS_ERR(gpioled.class)){
        goto del_cdev;
    }

    /* 5、创建设备 */
    gpioled.device = device_create(gpioled.class, NULL, gpioled.devid, NULL, GPIOLED_NAME);
    if(IS_ERR(gpioled.device))
        goto destroy_class;

    return ret;
destroy_class:
    class_destroy(gpioled.class);
del_cdev:
    cdev_del(&gpioled.cdev);
del_unregister:
    unregister_chrdev_region(gpioled.devid, GPIOLED_CNT);
free_gpio:
    gpio_free(gpioled.led_gpio);
    return -EIO;
}

/*
 * @description 	: 驱动出口函数
 * @param 			: 无
 * @return			: 无
 */
static void __exit led_exit(void){
    
    /* 驱动卸载时关闭LED灯 */ 
    gpio_set_value(gpioled.led_gpio, 1); /* 关闭 LED 灯 */

    /* 注销字符设备驱动 */
    cdev_del(&gpioled.cdev); /* 删除 cdev */
    unregister_chrdev_region(gpioled.devid, GPIOLED_CNT);

    /* 删除设备还要用到gpioled.class，这里先删除设备最后再删除类 */
    device_destroy(gpioled.class, gpioled.devid);
    /* 删除类 */
    class_destroy(gpioled.class);
    /* 释放 GPIO */
    gpio_free(gpioled.led_gpio); 
}


/* 注册驱动和卸载驱动 */
module_init(led_init);   /* 入口 */
module_exit(led_exit);   /* 出口 */
MODULE_LICENSE("GPL");
MODULE_AUTHOR("ALIENTEK");
MODULE_INFO(intree, "Y");
