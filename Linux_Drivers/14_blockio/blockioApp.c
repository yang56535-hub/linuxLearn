#include "stdio.h"
#include "unistd.h"
#include "sys/types.h"
#include "sys/stat.h"
#include "fcntl.h"
#include "stdlib.h"
#include "string.h"
/***************************************************************
Copyright © ALIENTEK Co., Ltd. 1998-2029. All rights reserved.
文件名 : keyApp.c
作者 : 杨中凡
版本 : V1.0
描述 : Linux 阻塞IO实验
其他 : 无
使用方法 ： ./blockioApp /dev/key
论坛 : www.openedv.com
日志 : 初版 V1.0 2025/09/30 
***************************************************************/

/*
 * @description : main 主程序
 * @param - argc : argv 数组元素个数
 * @param - argv : 具体参数
 * @return : 0 成功;其他 失败
 */

int main(int argc, char *argv[])
{
    int fd, ret;
    char *filename;
    int keyvalue;

    if(argc != 2){
        printf("Usage:\n"
            "\t./blockioApp /dev/key\n"
        );
        return -1;
    }

    filename = argv[1];
    /* 打开驱动文件 */
    fd = open(filename, O_RDWR); /* 以读写的方式打开文件 */
    if(fd < 0){
        printf("ERROR: file %s open failed!\r\n", filename);
        return -1;
    }

    /* 循环读取按键值数据！ */
    while(1) {
        read(fd, &keyvalue, sizeof(keyvalue));
        if(keyvalue == 0)
            printf("key Press, value = %#X\r\n", keyvalue);/* 按下 */
        else if(1 == keyvalue)
            printf("Key Release, value = %#X\r\n", keyvalue);/* 释放 */
    }

    ret= close(fd); /* 关闭文件 */
    if(ret < 0){
        printf("file %s close failed!\r\n", argv[1]);
        return -1;
    }
    return 0;
}