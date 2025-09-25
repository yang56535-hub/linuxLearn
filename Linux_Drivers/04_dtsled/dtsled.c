#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/ide.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/of.h>
#include <linux/of_address.h>

#define DTSLED_CNT 1            /* 设备号个数 */
#define DTSLED_NAME "dtsled"    /* 名字 */
#define LEDOFF 0 /* 关灯 */
#define LEDON 1 /* 开灯 */

/* 映射后的寄存器虚拟地址指针 */
static void __iomem *MPU_AHB4_PERIPH_RCC_PI;
static void __iomem *GPIOI_MODER_PI;
static void __iomem *GPIOI_OTYPER_PI;
static void __iomem *GPIOI_OSPEEDR_PI;
static void __iomem *GPIOI_PUPDR_PI;
static void __iomem *GPIOI_BSRR_PI;

/* dtsled设备结构体 */
struct dtsled_dev{
    dev_t devid;            /* 设备号 */
    struct cdev cdev;       /* 字符设备 */
    struct class *class;    /* 类 */
    struct device *device;   /* 设备 */
    int major;              /* 主设备号 */
    int minor;              /* 次设备号 */
    struct device_node *nd; /* 设备节点 */
};

struct  dtsled_dev dtsled;  // led设备

/*
 * @description : LED 打开/关闭
 * @param - sta : LEDON(0) 打开 LED， LEDOFF(1) 关闭 LED
 * @return : 无
 */
void led_switch(u8 sta)
{
    u32 val = 0;
    if(sta == LEDON) {
        val = readl(GPIOI_BSRR_PI);
        val |= (1 << 16);
        writel(val, GPIOI_BSRR_PI);
    }else if(sta == LEDOFF) {
        val = readl(GPIOI_BSRR_PI);
        val|= (1 << 0);
        writel(val, GPIOI_BSRR_PI);
    }
}

/*
* @description : 取消映射
* @return : 无
*/
void led_unmap(void)
{
    /* 取消映射 */
    iounmap(MPU_AHB4_PERIPH_RCC_PI);
    iounmap(GPIOI_MODER_PI);
    iounmap(GPIOI_OTYPER_PI);
    iounmap(GPIOI_OSPEEDR_PI);
    iounmap(GPIOI_PUPDR_PI);
    iounmap(GPIOI_BSRR_PI);
}

static int dtsled_open(struct inode *inode, struct file *filp)
{
	int ret = 0;
    filp->private_data = &dtsled; /* 设置私有数据 */
    //printk("led_open\r\n");
    return ret;
}
static int dtsled_release(struct inode *inode, struct file *filp)
{
	int ret = 0;

    //printk("led_release\r\n");
    return ret;
}

static ssize_t dtsled_read(struct file *filp, char __user *buf, size_t cnt, loff_t *offt)
{
    return 0;
}

/*
 * @description  : 向设备写数据
 * @param - filp : 设备文件，表示打开的文件描述符
 * @param - buf  : 要写给设备写入的数据
 * @param - cnt  : 要写入的数据长度
 * @param - offt : 相对于文件首地址的偏移
 * @return       : 写入的字节数，如果为负值，表示写入失败
 */
static ssize_t dtsled_write(struct file *filp, const char __user *buf,
			  size_t cnt, loff_t *offt)
{

    // 从 filp->private_data 中获取在 open 时存入的设备结构体指针
    // struct dtsled_dev *dev = (struct dtsled_dev *)filp->private_data;
    // 然后就可以使用 dev->major, dev->minor 等信息

    int ret = 0;
    unsigned char databuf[1];
    unsigned char ledstat;

    ret = copy_from_user(databuf, buf, cnt);
    if(ret < 0)
    {
        printk("kernel write failed!\r\n");
        ret = -EFAULT;
    }
    
    ledstat = databuf[0];   /* 获取到应用传递进来的开关灯状态 */

    if(ledstat == LEDON){   /* 开灯 */
        led_switch(LEDON);
    }else if(ledstat == LEDOFF){    /* 关灯 */
        led_switch(LEDOFF);
    }

    return ret;
}

/* dtsled字符设备操作集 */
static struct file_operations dtsled_fops = {
	.owner =   THIS_MODULE,
	.open =    dtsled_open,
    .read =    dtsled_read,
	.release = dtsled_release,
	.write =   dtsled_write,
};


/*
 * @description 	: 驱动入口函数
 * @param 			: 无
 * @return			: 无
 */
static int __init dtsled_init(void)
{
    u32 val = 0;
    int ret = 0;
    u32 regdata[12];
    const char *str;
    struct property *proper;

    /* 获取设备树中的属性数据 */
    /* 1、获取设备节点： stm32mp1_led */
    dtsled.nd = of_find_node_by_path("/stm32mp1_led");
    if(dtsled.nd == NULL){
        printk("stm32mp1_led node nost find!\r\n");
        return -EINVAL;
    }else{
        printk("stm32mp1_led node find!\r\n");
    }

    /* 2、获取 compatible 属性内容 */
    proper = of_find_property(dtsled.nd, "compatible", NULL);
    if(proper == NULL) {
        printk("compatible property find failed\r\n");
    } else {
        printk("compatible = %s\r\n", (char*)proper->value);
    }

    /* 3、获取 status 属性内容 */
    ret = of_property_read_string(dtsled.nd, "status", &str);
    if(ret < 0){
        printk("status read failed!\r\n");
    } else {
        printk("status = %s\r\n",str);
    }

    /* 4、获取 reg 属性内容 */
    ret = of_property_read_u32_array(dtsled.nd, "reg", regdata, 12);
    if(ret < 0) {
        printk("reg property read failed!\r\n");
    } else {
        u8 i = 0;
        printk("reg data:\r\n");
        for(i = 0; i < 12; i++)
            printk("%#X ", regdata[i]);
        printk("\r\n");
    }

    /* 初始化 LED */
    /* 1、寄存器地址映射 */
    MPU_AHB4_PERIPH_RCC_PI = of_iomap(dtsled.nd, 0);
    GPIOI_MODER_PI = of_iomap(dtsled.nd, 1);
    GPIOI_OTYPER_PI = of_iomap(dtsled.nd, 2);
    GPIOI_OSPEEDR_PI = of_iomap(dtsled.nd, 3);
    GPIOI_PUPDR_PI = of_iomap(dtsled.nd, 4);
    GPIOI_BSRR_PI = of_iomap(dtsled.nd, 5);

    /* 2、使能GPIOI时钟 */
    val = readl(MPU_AHB4_PERIPH_RCC_PI);
    val &= ~(0x1 << 8); /* 清除bit8以前的设置，&为按位与 */
    val |= (0x1 << 8);  /* 设置新值 */
    writel(val, MPU_AHB4_PERIPH_RCC_PI);

    /* 3、设置PI0通用的输出模式。*/
    val = readl(GPIOI_MODER_PI);
    val &= ~(0x3 << 0); /* bit0:1清零   */
    val |= (0x1 << 0); /* bit0:1设置01 */
    writel(val,GPIOI_MODER_PI);

    /* 4、设置PI0为推挽模式。*/
    val = readl(GPIOI_OTYPER_PI);
    val &= ~(0x1 << 0); /* bit0清零，设置为上拉 */
    writel(val, GPIOI_OTYPER_PI);

    /* 5、设置PI0为超高速。*/
    val = readl(GPIOI_OSPEEDR_PI);
    val &= ~(0x3 << 0); /* bit0:1 清零      */
    val |= (0x3 << 0);  /* bit0:1 设置为11  */

    /* 6、设置PI0为上拉。*/
    val = readl(GPIOI_PUPDR_PI);
    val &= ~(0x3 << 0); /* bit0:1 清零      */
    val |= (0x1 << 0);  /* bit0:1 设置为01  */
    writel(val,GPIOI_PUPDR_PI);

    /* 7、将GPIOI_0默认输出高电平，关闭LED灯 */
    val = readl(GPIOI_BSRR_PI);
    val &= ~(0x1 << 0);  /* 将bit0清零 */
    val |= (0x1 << 0);  /* 将bit0设置为1 */
    writel(val, GPIOI_BSRR_PI);

    /* 注册字符设备 */
    /* 1、申请设备号 */
    dtsled.major = 0;   /* 设备号由内核分配 */
    if(dtsled.major){   /* 定义了设备号 */
        dtsled.devid = MKDEV(dtsled.major,0);
        ret = register_chrdev_region(dtsled.devid, DTSLED_CNT, DTSLED_NAME);
        if(ret < 0){
            pr_err("cannot register %s char driver[ret=%d]\n",DTSLED_NAME, DTSLED_CNT);
            goto fail_map;
        }
    }else{              /* 没有给定设备号 */
        ret = alloc_chrdev_region(&dtsled.devid, 0, DTSLED_CNT, DTSLED_NAME);
        if(ret < 0){
            pr_err("%s Couldn't alloc_chrdev_region, ret=%d\r\n", DTSLED_NAME, ret);
            goto fail_map;
        }
        dtsled.major = MAJOR(dtsled.devid);
        dtsled.minor = MINOR(dtsled.devid);
    }
    printk("dtsled major=%d,minor=%d\r\n",dtsled.major, dtsled.minor);

    /* 2、初始化 cdev */
    dtsled.cdev.owner = THIS_MODULE;
    cdev_init(&dtsled.cdev, &dtsled_fops);

    /* 3、添加一个 cdev  */
    ret = cdev_add(&dtsled.cdev, dtsled.devid, DTSLED_CNT);
    if(ret < 0)
        goto del_unregister;
    
    /* 自动创建设备节点需要如下两步 */
    /* 4、创建类 */
    dtsled.class = class_create(THIS_MODULE,DTSLED_NAME);
    if(IS_ERR(dtsled.class)){
        ret = PTR_ERR(dtsled.class);
        goto del_cdev;
    }

    /* 5、创建设备 */
    dtsled.device = device_create(dtsled.class, NULL, dtsled.devid, NULL, DTSLED_NAME);
    if(IS_ERR(dtsled.device)){
        ret = PTR_ERR(dtsled.device);
        goto destory_class;
    }

    return 0;
destory_class:
    class_destroy(dtsled.class);
del_cdev:
    cdev_del(&dtsled.cdev);
del_unregister:
    unregister_chrdev_region(dtsled.devid, DTSLED_CNT);
fail_map:
    led_unmap();
    return -EIO;
}

/*
 * @description 	: 驱动出口函数
 * @param 			: 无
 * @return			: 无
 */
static void __exit dtsled_exit(void)
{
    unsigned int val = 0;
    printk("dtsled_exit\r\n");
    /* 将GPIOI_0默认输出高电平，驱动卸载时关闭LED灯 */
    val = readl(GPIOI_BSRR_PI);
    val &= ~(0x1 << 0);  /* 将bit0清零 */
    val |= (0x1 << 0);  /* 将bit0设置为1 */
    writel(val, GPIOI_BSRR_PI);
    
    /* 取消映射 */
    led_unmap();
    
    /* 注销字符设备驱动 */
    /* 删除 cdev */
    cdev_del(&dtsled.cdev); /* 删除 cdev */
    unregister_chrdev_region(dtsled.devid, DTSLED_CNT); /* 注销 */

    /* 删除设备还要用到dtsled.class，这里先删除设备最后再删除类 */
    device_destroy(dtsled.class, dtsled.devid);
    /* 删除类 */
    class_destroy(dtsled.class);
}

/* 注册驱动和卸载驱动 */
module_init(dtsled_init);   /* 入口 */
module_exit(dtsled_exit);   /* 出口 */
MODULE_LICENSE("GPL");
MODULE_AUTHOR("ALIENTEK");
MODULE_INFO(intree, "Y");