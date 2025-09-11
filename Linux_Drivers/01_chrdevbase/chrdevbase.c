#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/ide.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>

#define CHRDEVBASE_MAJOR    200
#define CHRDEVBASE_NAME     "chrdevbase"

static char readbuf[100]; /* 读缓冲区 */
static char writebuf[100];/* 写缓冲区 */

static char kerneldata[] = {"kernel data!"}; /* 应用从内核读取到的数据 */



/*
ssize_t (*read) (struct file *, char __user *, size_t, loff_t *);
ssize_t (*write) (struct file *, const char __user *, size_t, loff_t *);
int (*open) (struct inode *, struct file *);
int (*release) (struct inode *, struct file *);*/

static int chrdevbase_open(struct inode *inode, struct file *filp)
{
	int ret = 0;

    //printk("chrdevbase_open\r\n");
    return ret;
}

static int chrdevbase_release(struct inode *inode, struct file *filp)
{
	int ret = 0;

    //printk("chrdevbase_release\r\n");
    return ret;
}

/*
 * @description  : 向设备写数据
 * @param - filp : 设备文件，表示打开的文件描述符
 * @param - buf  : 要写给设备写入的数据
 * @param - cnt  : 要写入的数据长度
 * @param - offt : 相对于文件首地址的偏移
 * @return       : 写入的字节数，如果为负值，表示写入失败
 */
static ssize_t chrdevbase_write(struct file *filp, const char __user *buf,
			  size_t cnt, loff_t *offt)
{
    int ret = 0;

    memcpy(readbuf, kerneldata, sizeof(kerneldata));

    ret = copy_from_user(writebuf, buf, cnt);
    if(ret == 0)
    {
        printk("kernel recevedata:%s\r\n", writebuf);
    }
    else
    {
        printk("kernel recevedata failed!\r\n");
    }
    
    //printk("chrdevbase_write\r\n");
    return 0;
}

/*
 * @description     : 从设备读取数据
 * @param - filp    : 要打开的设备文件(文件描述符)
 * @param - buf     : 返回给用户空间的数据缓冲区
 * @param - cnt     : 要读取的数据长度
 * @param - offt    : 相对于文件首地址的偏移
 * @return : 读取的字节数，如果为负值，表示读取失败
 */
static ssize_t chrdevbase_read(struct file *filp, char __user *buf, size_t cnt,
			 loff_t *offt)
{
    int ret = 0;

    memcpy(readbuf, kerneldata, sizeof(kerneldata));
    
    ret = copy_to_user(buf, readbuf, cnt);
    if(ret == 0)
    {
        printk("kernel senddata ok!\r\n");
    }
    else
    {
        printk("kernel senddata failed!\r\n");
    }
    //printk("chrdevbase_read\r\n");
    return 0;
}

const struct file_operations chrdevbase_fops = {
	.owner =   THIS_MODULE,
	.open =    chrdevbase_open,
	.release = chrdevbase_release,
	.write =   chrdevbase_write,
	.read =    chrdevbase_read,
};

//* 入口函数 */
static int __init chrdevbase_init(void)
{
    int ret = 0;

    printk("chrdevbase_init\r\n");

    /* 向内核注册字符设备 */
    //static inline int register_chrdev(unsigned int major, const char *name,
	//			  const struct file_operations *fops)
    ret = register_chrdev(CHRDEVBASE_MAJOR, CHRDEVBASE_NAME, &chrdevbase_fops);
    if (ret < 0) {
		printk("chrdevbase driver register failed!\r\n");
	}
    return ret;
}

/* 出口函数 */
static void __exit chrdevbase_exit(void)
{
    printk("chrdevbase_exit\r\n");
    //* 注销字符设备 */
    //static inline void unregister_chrdev(unsigned int major, const char *name)
    unregister_chrdev(CHRDEVBASE_MAJOR, CHRDEVBASE_NAME);
}


/* 驱动的注册与卸载 */
module_init(chrdevbase_init);   /* 入口函数 */
module_exit(chrdevbase_exit);   /* 出口函数 */

/* 添加lience和作者信息 */
MODULE_LICENSE("GPL");
MODULE_AUTHOR("zuozhongkai");
MODULE_INFO(intree, "Y");