#include "stdio.h"
#include "unistd.h"
#include "sys/types.h"
#include "sys/stat.h"
#include "fcntl.h"
#include "stdlib.h"
#include "string.h"
#include <signal.h>

/***************************************************************
Copyright © ALIENTEK Co., Ltd. 1998-2029. All rights reserved.
文件名 : asyncnotiApp.c
作者 : 杨中凡
版本 : V1.0
描述 : Linux 阻塞IO实验
其他 : 无
使用方法 ： ./asyncnotiApp /dev/key
论坛 : www.openedv.com
日志 : 初版 V1.0 2025/10/09 
***************************************************************/

static int fd;

/*
 * SIGIO 信号处理函数
 * @param – signum : 信号值
 * @return : 无
 */
static void sigio_signal_func(int signum)
{
    unsigned int key_val = 0;

    read(fd, &key_val, sizeof(unsigned int));
    if (0 == key_val)
        printf("Key Press\n");
    else if (1 == key_val)
        printf("Key Release\n");
}

/*
 * @description : main 主程序
 * @param - argc : argv 数组元素个数
 * @param - argv : 具体参数
 * @return : 0 成功;其他 失败
 */

int main(int argc, char *argv[])
{
    int flags = 0;
    int ret; 

    if(argc != 2){
        printf("Usage:\n"
            "\t./asyncKeyApp /dev/key\n"
        );
        return -1;
    }

    /* 打开设备 */
    fd = open(argv[1], O_RDONLY | O_NONBLOCK);
    if(0 > fd) {
        printf("ERROR: %s file open failed!\n", argv[1]);
        return -1;
    }

    /* 设置信号 SIGIO 的处理函数 */
    signal(SIGIO, sigio_signal_func);
    fcntl(fd, F_SETOWN, getpid()); /* 将当前进程的进程号告诉给内核 */
    flags = fcntl(fd, F_GETFD); /*获取当前的进程状态 */
    fcntl(fd, F_SETFL, flags | FASYNC);/* 设置进程启用异步通知功能 */

    /* 循环轮询读取按键数据 */
    for ( ; ; ) {
        sleep(2);
    }

    ret = close(fd); /* 关闭文件 */
    if(ret < 0){
        printf("file %s close failed!\r\n", argv[1]);
        return -1;
    }
    return 0;
}