/**
 * @file zigbee-protocol.c
*/

#include "zigbee-protocol.h"
#include "../../src/log.h"
#include "../common.h"
#include <fcntl.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <stdlib.h>
#include <linux/serial.h>
#include <linux/ioctl.h>
#include <sys/ioctl.h>

int uart_init(const char *path)
{
    int fd = open(path, O_RDWR | O_NOCTTY | O_NONBLOCK | O_NDELAY);
    if (fd == -1)
    {
        logp(path);
        return -1;
    }
    //改为非阻塞
    fcntl(fd, F_SETFL, 0);
    fcntl(fd, F_SETFL, O_NOCTTY);

    struct termios opts;
    memset(&opts, 0, sizeof(struct termios));

    tcgetattr(fd, &opts); //把uart接口的原配置值获取出来，在此基础上修改.
    //配置波特率
    cfsetispeed(&opts, B9600);
    cfsetospeed(&opts, B9600);

    opts.c_cflag |= CLOCAL | CREAD; //怱略modem的控制线，打开接收器

    // 8N1
    opts.c_cflag &= ~PARENB; // 不用校验
    opts.c_cflag &= ~CSTOPB; //用一位停止位
    opts.c_cflag &= ~CSIZE;  //把数据位的配置先清除
    opts.c_cflag |= CS8;     //8位数据位

    //关闭硬件流控
    //opts.c_cflag &= ~CRTSCTS;

    // raw input: 即让uart接收到什么样的数据不要自己处理，直接交过来
    opts.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);

    // raw output: 让uart直接发出数据.
    opts.c_oflag &= ~OPOST;
    //清空数据
    tcflush(fd,TCIFLUSH);
    //设置
    tcsetattr(fd, TCSANOW, &opts);
    return fd;
}

zigbee_uart_t *zigbee_uart_init()
{
    zigbee_uart_t *uart = malloc(sizeof(zigbee_uart_t));
    if (uart == NULL)
        return NULL;
    uart->readStatus = READ_DEV;
    uart->destlen = 0;
    return uart;
}

uint32_t makeHeader(char *buf, uint16_t srcAddr, uint16_t datalen)
{
    char *p = buf;
    memcpy(p, DEV_MAGIC, strlen(DEV_MAGIC));
    p += strlen(DEV_MAGIC);
    memcpy(p, &srcAddr, sizeof(uint16_t));
    p += sizeof(uint16_t);
    memcpy(p, &datalen, sizeof(uint16_t));
    p += sizeof(uint16_t);
    return p - buf;
}

bool zigbee_isAddr(zigbee_dev_t *dev, uint16_t *addr)
{
    return dev->addr == *addr;
}

void getCompleteFrame(uint8_t *inBuf, uint16_t inCnt, zigbee_uart_t *uart)
{
    int i;
    if (uart->readStatus == READ_FINSH)
    {
        uart->readStatus = READ_DEV;
        uart->destlen = 0;
        getCompleteFrame(uart->tmpbuf, uart->tmplen, uart);
    }
    for (i = 0; i < inCnt; i++)
    {
        switch (uart->readStatus)
        {
        case READ_DEV: //读取头部"DEV"
            if ((inBuf[i] == 'D' && uart->destlen == 0) || (inBuf[i] == 'E' && uart->destlen == 1))
            {
                uart->outbuf[uart->destlen++] = inBuf[i];
            }
            else if (inBuf[i] == 'V' && uart->destlen == 2)
            {
                uart->outbuf[uart->destlen++] = inBuf[i];
                uart->readStatus = READ_ADDR;
            }
            else
            {
                uart->destlen = 0;
            }
            break;
        case READ_ADDR: //读取ADDR
            uart->outbuf[uart->destlen++] = inBuf[i];
            if (uart->destlen == 5) //读完addr
                uart->readStatus = READ_LEN;
            break;
        case READ_LEN: //读取长度
            uart->outbuf[uart->destlen++] = inBuf[i];
            if (uart->destlen == 7) //读完len
            {
                uart->len = *(uint16_t *)(uart->outbuf + 5);
                logd("<getCompleteFrame>pack len: %d\n", uart->len);
                if(uart->len > PACKET_MAX)  
                {
                    uart->readStatus = READ_DEV;
                    uart->destlen = 0;
                }
                //uart->len = (uart->outbuf[5] << 8) + uart->outbuf[6];
                else uart->readStatus = READ_BODY;
            }
            break;
        case READ_BODY: //读取数据部分
            uart->outbuf[uart->destlen++] = inBuf[i];
            //printf("[%02x %02x] %d %d\n", uart->outbuf[5], uart->outbuf[6], uart->len, uart->destlen);
            if (uart->destlen - uart->len == HEADER_LEN + CHKSUM_LEN) //包完整
            {
                uart->readStatus = READ_FINSH;
                uart->tmplen = inCnt - i + 1; //缓冲到下次再读取
                memcpy(uart->tmpbuf, &inBuf[i], uart->tmplen);
                return;
            }
            break;
        defualt:
            break;
        }
    }
}

uint8_t uart_chkSum(uint8_t *buf, uint16_t buflen)
{
    uint8_t chkSum = 0;
    for (uint16_t i = 0; i < buflen; i++)
    {
        chkSum += buf[i];
    }
    return chkSum;
}

bool zigbee_isId(zigbee_dev_t *dev, char *id)
{
    return strncmp(dev->id, id, ID_LEN) == 0;
}