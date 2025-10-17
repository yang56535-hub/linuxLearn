#include "stdio.h"
#include "unistd.h"
#include "sys/types.h"
#include "sys/stat.h"
#include "sys/ioctl.h"
#include "fcntl.h"
#include "stdlib.h"
#include "string.h"
#include <poll.h>
#include <sys/select.h>
#include <sys/time.h>
#include <signal.h>
#include <fcntl.h>

/***************************************************************
Copyright © ALIENTEK Co., Ltd. 1998-2029. All rights reserved.
文件名 : asyncnotiApp.c
作者 : 杨中凡
版本 : V1.0
描述 : Linux I2C驱动实验
其他 : 无
使用方法 ： ./ap3216cApp /dev/ap3216c
论坛 : www.openedv.com
日志 : 初版 V1.0 2025/10/16 
***************************************************************/

int main(int argc, char *argv[])
{
    int fd;
    char *filename;
    unsigned short databuf[3];
    unsigned short ir, als, ps;
    int ret = 0;

    if (argc != 2) {
        printf("Error Usage!\r\n");
        return -1;
    }

    filename = argv[1];
    fd = open(filename, O_RDWR);
    if(fd < 0) {
        printf("can't open file %s\r\n", filename);
        return -1;
    }

    while (1) {
        ret = read(fd, databuf, sizeof(databuf));
        if(ret == 0) { /* 数据读取成功 */
            ir = databuf[0]; /* ir 传感器数据 */
            als = databuf[1]; /* als 传感器数据 */
            ps = databuf[2]; /* ps 传感器数据 */
            printf("ir = %d, als = %d, ps = %d\r\n", ir, als, ps);
        }
        usleep(200000); /*100ms */
    }
    close(fd); /* 关闭文件 */
    return 0;
}