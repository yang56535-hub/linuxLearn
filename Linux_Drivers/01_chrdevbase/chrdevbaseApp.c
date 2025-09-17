#include "stdio.h"
#include "unistd.h"
#include "sys/types.h"
#include "sys/stat.h"
#include "fcntl.h"
#include "stdlib.h"
#include "string.h"

static char usrdata[] = {"usr data!"};

/*
 * APP运行命令：./chrdevbaseAPP filename <1>|<2> 如果是1表示读数据，如果是2表示写数据
 */
int main (int argc, char *argv[])
{
    int fd, retvalue;

    char readbuf[100], writebuf[100];

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

    if(atoi(argv[2]) == 1) {/* 从驱动文件读取数据 */
        retvalue = read(fd, readbuf, 50);
        if(retvalue < 0){
            printf("read file %s failed!/r/n", filename);
        }else{
            printf("usr read data: %s\r\n", readbuf);
        }
    }

    if(atoi(argv[2]) == 2){ /* 向驱动文件写数据 */
        memcpy(writebuf, usrdata, sizeof(usrdata));

        retvalue = write(fd, writebuf, 50);

        if(retvalue < 0){
            printf("write file %s \r\n", filename);
        }
    }

    /* 关闭文件 */
    retvalue = close(fd);
    if(retvalue < 0){
        printf("Can't close file %s\r\n", filename);
    }

    return retvalue;

}