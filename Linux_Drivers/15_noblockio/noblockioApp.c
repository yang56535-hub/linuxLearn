#include "stdio.h"
#include "unistd.h"
#include "sys/types.h"
#include "sys/stat.h"
#include "fcntl.h"
#include "stdlib.h"
#include "string.h"
#include "poll.h"

/***************************************************************
Copyright © ALIENTEK Co., Ltd. 1998-2029. All rights reserved.
文件名 : keyApp.c
作者 : 杨中凡
版本 : V1.0
描述 : Linux 阻塞IO实验
其他 : 无
使用方法 ： ./noblockioApp /dev/key
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
    // fd_set readfds;
    struct pollfd fds;
    // struct timeval timeout; /* 超时结构体 */

    if(argc != 2){
        printf("Usage:\n"
            "\t./noblockioApp /dev/key\n"
        );
        return -1;
    }

    filename = argv[1];
    /* 打开驱动文件 */
    fd = open(argv[1], O_RDONLY | O_NONBLOCK); /* 非阻塞式访问  */
    if(fd < 0){
        printf("ERROR: file %s open failed!\r\n", filename);
        return -1;
    }

    // FD_ZERO(&readfds);      /* 清除 readfds */
    // FD_SET(fd, &readfds);   /* 将 fd 添加到 readfds 里面 */
#if 0
    /* 循环轮询读取按键数据 */
    for ( ; ; ) {
        ret = select(fd + 1, &readfds, NULL, NULL, NULL);
        switch (ret) {

            case 0: /* 超时 */
                printf("timeout!\r\n");
                break;

            case -1: /* 错误 */
                printf("error!\r\n");
                break;

            default:    /* 可以读取数据 */
                if(FD_ISSET(fd, &readfds)) {    /* 判断是否为 fd 文件描述符 */
                    ret = read(fd, &keyvalue, sizeof(int)); // 返回值为0表示读取成功
                    printf("ret = %d\r\n", ret);
                    if (0 == keyvalue)
                        printf("Key Press\n");
                    else if (1 == keyvalue)
                        printf("Key Release\n");

                }
                break;
        }
    }
#endif

    fds.fd = fd;
    fds.events = POLLIN;

    /* 循环轮训读取按键数据 */
    for ( ; ; ) {
        ret = poll(&fds, 1, 5000);   /* 超时500ms */
        switch (ret) {
        
            case 0: /* 超时 */
                /* 用户自定义超时处理 */
                printf("poll timeout!\r\n");
                break;
            
            case -1: /* 错误 */
                /* 用户自定义错误处理 */
                break;
            
            default:
                if(fds.revents | POLLIN) {  /* 可读取 */
                    read(fd, &keyvalue, sizeof(int));
                    if (0 == keyvalue)
                        printf("Key Press\n");
                    else if (1 == keyvalue)
                        printf("Key Release\n");
                }
                
                break;
        }
    }

    ret= close(fd); /* 关闭文件 */
    if(ret < 0){
        printf("file %s close failed!\r\n", argv[1]);
        return -1;
    }
    return 0;
}