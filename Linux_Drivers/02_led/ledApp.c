#include "stdio.h"
#include "unistd.h"
#include "sys/types.h"
#include "sys/stat.h"
#include "fcntl.h"
#include "stdlib.h"
#include "string.h"

#define LEDOFF 0
#define LEDON 1

/*
 * APP运行命令：./chrdevbaseAPP filename <1>|<0> 如果是1表示打开LED，如果是0表示关闭LED
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