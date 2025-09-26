#include "stdio.h"
#include "unistd.h"
#include "sys/types.h"
#include "sys/stat.h"
#include "fcntl.h"
#include "stdlib.h"
#include "string.h"

/***************************************************************
Copyright © ALIENTEK Co., Ltd. 1998-2029. All rights reserved.
文件名 : beepApp.c
作者 : 杨中凡
版本 : V1.0
描述 : beep 测试 APP。
其他 : 无
使用方法 ：   ./beepApp /dev/beep 0 关闭蜂鸣器
            ./beepApp /dev/beep 1 打开蜂鸣器
论坛 : www.openedv.com
日志 : 改版 V1.0 2025/09/26
***************************************************************/

#define BEEPOFF 0
#define BEEPON 1

/*
 * APP运行命令：./beepApp filename <1>|<0> 如果是1表示打开beep，如果是0表示关闭beep
 */
int main (int argc, char *argv[])
{
    int fd, retvalue;

    unsigned char databuf[1];

    char *filename;

    if(argc != 3)
    {
        printf("Error Usage!\r\n");
        return -1;
    }

    filename = argv[1];

    /* 打开文件 */
    fd = open(filename, O_RDWR); /* 以读写的方式打开文件 */
    if (fd < 0){
        printf("Can't open file %s\r\n", filename);
        return -1;
    }

    databuf[0] = atoi(argv[2]); /* 写入的数据，是数字的。表示打开或关闭 */
    retvalue = write(fd, databuf, 1);

    if(retvalue < 0){
        printf("write file %s \r\n", filename);
        close(fd);
        return -1;
    }

    /* 关闭文件 */
    retvalue = close(fd);
    if(retvalue < 0){
        printf("Can't close file %s\r\n", filename);
        return -1;
    }

    return 0;

}