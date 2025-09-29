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
#include <linux/semaphore.h>

/***************************************************************
Copyright © ALIENTEK Co., Ltd. 1998-2029. All rights reserved.
文件名 : key.c
作者 : 杨中凡
版本 : V1.0
描述 : Linux 按键输入驱动实验
其他 : 无
论坛 : www.openedv.com
日志 : 改版 V1.0 2025/09/27 
***************************************************************/

#define KEY_CNT 1           /* 设备号个数 */
#define KEY_NAME "key"      /* 名字 */

/* 定义按键值 */
#define KEY0VALUE 0XF0 /* 按键值 */
#define INVAKEY 0X00 /* 无效的按键值 */

/* key 设备结构体 */
struct key_dev{
    dev_t devid;            /* 设备号 */
    struct cdev cdev;       /* cdev */
    struct class *class;    /* 类 */
    struct device *device;  /* 设备 */
    int major;              /* 主设备号 */
    int minor;
    struct device_node *nd; /* 设备节点 */
    int key_gpio;           /* led所使用的GPIO编号 */
    atomic_t keyvalue;      /* 按键值 */
};

static struct key_dev keydev;/* key 设备 */

/*
 * @description : 初始化按键 IO， open 函数打开驱动的时候
 * 初始化按键所使用的 GPIO 引脚。
 * @param : 无
 * @return : 无
 */
static int keyio_init(void)
{
    int ret;
    const char *str;
    /* 设置 key 所使用的 GPIO */
    /* 1、获取设备节点： keydev */
    keydev.nd = of_find_node_by_path("/key");
    if(keydev.nd == NULL) {
        printk("keydev node not find!\r\n");
        return -EINVAL;
    }

    /* 2.读取 status 属性 */
    ret = of_property_read_string(keydev.nd, "status", &str);
    if(ret < 0)
        return -EINVAL;

    if (strcmp(str, "okay"))
        return -EINVAL;

    /* 3、获取 compatible 属性值并进行匹配 */
    ret = of_property_read_string(keydev.nd, "compatible", &str);
    if(ret < 0) {
        printk("keydev: Failed to get compatible property\n");
        return -EINVAL;
    }

    if (strcmp(str, "alientek,key")) {
        printk("keydev: Compatible match failed\n");
        return -EINVAL;
    }

    /* 4、 获取设备树中的 gpio 属性，得到 KEY0 所使用的 KYE 编号 */
    keydev.key_gpio = of_get_named_gpio(keydev.nd, "key-gpio", 0);
    if(keydev.key_gpio < 0) {
        printk("can't get key-gpio");
        return -EINVAL;
    }
    printk("key-gpio num = %d\r\n", keydev.key_gpio);

    /* 5.向 gpio 子系统申请使用 GPIO */
    ret = gpio_request(keydev.key_gpio, "KEY0");
    if (ret) {
        printk(KERN_ERR "keydev: Failed to request key-gpio\n");
        return ret;
    }

    /* 6、设置 PG3 输入模式 */
    ret = gpio_direction_input(keydev.key_gpio);
    if(ret < 0) {
        printk("can't set gpio!\r\n");
        return ret;
    }

    return 0;
}

/*
 * @description : 打开设备
 * @param – inode : 传递给驱动的 inode
 * @param - filp : 设备文件， file 结构体有个叫做 private_data 的成员变量
 * 一般在 open 的时候将 private_data 指向设备结构体。
 * @return : 0 成功;其他 失败
 */
static int key_open(struct inode *inode, struct file *filp)
{
    int ret = 0;
    filp->private_data = &keydev; /* 设置私有数据 */

    ret = keyio_init(); /* 初始化按键 IO，将GPIO初始化放在open里，
                        应用程序每次打开按键驱动文件的时候都会初始化一次按键 IO */
    if (ret < 0) {

        return ret;
    }
    
    return 0;
}

/*
 * @description : 从设备读取数据
 * @param - filp : 要打开的设备文件(文件描述符)
 * @param - buf : 返回给用户空间的数据缓冲区
 * @param - cnt : 要读取的数据长度
 * @param - offt : 相对于文件首地址的偏移
 * @retu rn : 读取的字节数，如果为负值，表示读取失败
 */
static ssize_t key_read(struct file *filp, char __user *buf, size_t cnt, loff_t *offt)
{
    int ret = 0;
    int value;
    struct key_dev *dev = filp->private_data;
    if (gpio_get_value(dev->key_gpio) == 0) { /* 当 KEY0 按下以后 PG3 就是低电平 */
    while(!gpio_get_value(dev->key_gpio)); /* 等待按键释放 */
        atomic_set(&dev->keyvalue, KEY0VALUE);
    } else {
        atomic_set(&dev->keyvalue, INVAKEY); /* 无效的按键值 */
    }

    value = atomic_read(&dev->keyvalue);    /* 转成int型上报给应用 */
    ret = copy_to_user(buf, &value, sizeof(value));    

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
static ssize_t key_write(struct file *filp, const char __user *buf, size_t cnt, loff_t *offt)
{
    return 0;
}

/*
 * @description : 关闭/释放设备
 * @param – filp : 要关闭的设备文件(文件描述符)
 * @return : 0 成功;其他 失败
 */
static int key_release(struct inode *inode, struct file *filp)
{
    struct key_dev *dev = filp->private_data;
    /* 关闭驱动文件的时候释放gpio */
    gpio_free(dev->key_gpio);

    return 0;
}

/* 设备操作函数 */
static struct file_operations key_fops = {
    .owner = THIS_MODULE,
    .open = key_open,
    .read = key_read,
    .write = key_write,
    .release = key_release,
};

/*
 * @description 	: 驱动入口函数
 * @param 			: 无
 * @return			: 无
 */
static int __init mykey_init(void){
    int ret = 0;
    /* 1、初始化原子变量 */
    keydev.keyvalue= (atomic_t)ATOMIC_INIT(0);

    /* 2、原子变量初始值为 INVAKEY */
    atomic_set(&keydev.keyvalue, INVAKEY);

    /* 注册字符设备驱动 */
    /* 1、创建设备号 */
    keydev.major = 0;
    if(keydev.major){  /* 定义了设备号 */
        keydev.devid = MKDEV(keydev.major,0);
        ret = register_chrdev_region(keydev.devid, KEY_CNT, KEY_NAME);

        if(ret < 0){
            pr_err("cannot register %s char driver [ret=%d] \n", KEY_NAME, ret);
            return -EIO; 
            // goto free_gpio;
        }
    }else{
        ret = alloc_chrdev_region(&keydev.devid, 0, KEY_CNT, KEY_NAME); /* 申请设备号 */
        if(ret < 0){
            pr_err("%s Couldn't alloc_chrdev_region, ret=%d\r\n", KEY_NAME, ret);
            return -EIO;
            // goto free_gpio;
        }
        keydev.major = MAJOR(keydev.devid);   /* 获取分配好的主设备号 */
        keydev.minor = MINOR(keydev.devid);   /* 获取分配好的次设备号 */
    }
    printk("keydev major=%d,minor=%d\r\n", keydev.major, keydev.minor);

    /* 2、初始化cdev */
    keydev.cdev.owner = THIS_MODULE;
    cdev_init(&keydev.cdev, &key_fops);

    /* 3、添加一个cdev */
    cdev_add(&keydev.cdev, keydev.devid, KEY_CNT);
    if(ret < 0)
        goto del_unregister;

    /* 4、创建类 */
    keydev.class = class_create(THIS_MODULE, KEY_NAME);
    if(IS_ERR(keydev.class)){
        goto del_cdev;
    }

    /* 5、创建设备 */
    keydev.device = device_create(keydev.class, NULL, keydev.devid, NULL, KEY_NAME);
    if(IS_ERR(keydev.device))
        goto destroy_class;

    return ret;
destroy_class:
    class_destroy(keydev.class);
del_cdev:
    cdev_del(&keydev.cdev);
del_unregister:
    unregister_chrdev_region(keydev.devid, KEY_CNT);
    return -EIO;
// free_gpio:
//     gpio_free(keydev.key_gpio);
//     return -EIO;
}

/*
 * @description 	: 驱动出口函数
 * @param 			: 无
 * @return			: 无
 */
static void __exit mykey_exit(void){

    /* 注销字符设备驱动 */
    cdev_del(&keydev.cdev); /* 删除 cdev */
    unregister_chrdev_region(keydev.devid, KEY_CNT);

    /* 删除设备还要用到gpioled.class，这里先删除设备最后再删除类 */
    device_destroy(keydev.class, keydev.devid);
    /* 删除类 */
    class_destroy(keydev.class);
    
}


/* 注册驱动和卸载驱动 */
module_init(mykey_init);   /* 入口 */
module_exit(mykey_exit);   /* 出口 */
MODULE_LICENSE("GPL");
MODULE_AUTHOR("ALIENTEK");
MODULE_INFO(intree, "Y");
