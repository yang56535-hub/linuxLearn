#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <linux/input.h>

/*
 * @description : main 主程序
 * @param – argc : argv 数组元素个数
 * @param – argv : 具体参数
 * @return : 0 成功;其他 失败
 */
int main(int argc, char *argv[])
{
    int fd, ret;
    struct input_event ev;

    if(2 != argc) {
        printf("Usage:\n"
        "\t./keyinputApp /dev/input/eventX @ Open Key\n"
        );
        return -1;
    }

    /* 打开设备 */
    fd = open(argv[1], O_RDWR);
    if(0 > fd) {
        printf("Error: file %s open failed!\r\n", argv[1]);
        return -1;
    }

    /* 读取按键数据 */
    for ( ; ; ) {

        ret = read(fd, &ev, sizeof(struct input_event));
        if (ret) {
            switch (ev.type) {
                case EV_KEY: /* 按键事件 */
                    if (KEY_0 == ev.code) { /* 判断是不是 KEY_0 按键 */
                        if (ev.value) /* 按键按下 */
                            printf("Key0 Press\n");
                        else /* 按键松开 */
                            printf("Key0 Release\n");
                    }
                    break;

                /* 其他类型的事件，自行处理 */
                case EV_REL:
                    break;
                case EV_ABS:
                    break;
                case EV_MSC:
                    break;
                case EV_SW:
                    break;
            };
        }
        else {
            printf("Error: file %s read failed!\r\n", argv[1]);
            goto out;
        }
    }

out:
    /* 关闭设备 */
    close(fd);
    return 0;
}

