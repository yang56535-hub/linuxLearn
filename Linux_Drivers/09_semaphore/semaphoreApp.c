#include "stdio.h"
#include "unistd.h"
#include "sys/types.h"
#include "sys/stat.h"
#include "fcntl.h"
#include "stdlib.h"
#include "string.h"

/***************************************************************
Copyright © ALIENTEK Co., Ltd. 1998-2029. All rights reserved.
文件名 : semaphoreApp.c
作者 : 杨中凡
版本 : V1.0
描述 : 信号量实验测试 APP，测试信号量实验能不能实现一次只允许一个应用程序使用 LED。
其他 : 无
使用方法 ：   ./atomicApp /dev/gpioled 0 关闭 LED 灯
            ./atomicApp /dev/gpioled 1 打开 LED 灯
论坛 : www.openedv.com
日志 : 改版 V1.0 2025/09/27
***************************************************************/

#define LEDOFF 0
#define LEDON 1

/*
 * @description : main 主程序
 * @param - argc : argv 数组元素个数
 * @param - argv : 具体参数
 * @return : 0 成功;其他 失败
 */
int main (int argc, char *argv[])
{
    int fd, retvalue;
    unsigned char cnt = 0;
    unsigned char databuf[1];

    char *filename;

    if(argc != 3)
    {
        printf("Error Usage!\r\n");
        return -1;
    }

    filename = argv[1];

    /* 打开驱动文件 */
    fd = open(filename, O_RDWR); /* 以读写的方式打开文件 */
    if (fd < 0){
        printf("Can't open file %s\r\n", filename);
        return -1;
    }

    databuf[0] = atoi(argv[2]); /* 写入的数据，是数字的。表示打开或关闭 */
    retvalue = write(fd, databuf, 1);
    if(retvalue < 0){
        printf("LED Control Failed! %s \r\n", filename);
        close(fd);
        return -1;
    }

    /* 模拟占用 25S LED，比如亮25s内无法再次 */
    while(1) {
        sleep(5);
        cnt++;
        printf("App running times:%d\r\n", cnt);
        if(cnt >= 5) break;
    }

    /* 关闭文件 */
    retvalue = close(fd);
    if(retvalue < 0){
        printf("Can't close file %s\r\n", filename);
        return -1;
    }

    return 0;

}