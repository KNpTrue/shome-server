/**
 * event-callback.c
*/
#include "event-callback.h"
#include "event-who.h"
#include "../../src/warp.h"
#include "zigbee-protocol.h"
#include "../common.h"
#include "../../src/dev-encrypt.h"
#include "../../src/log.h"


#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define  SERVER_PORT  8003
#define  SERVER_ADDR  "192.168.1.103"
#define  DOMAIN  AF_INET

static node_t *zigbeeDevList_head = NULL;

#define _switchEventMode(_struct, _event) \
        ({(_struct)->event = (_event);\
        eventMod(_struct);})

int recvUart_cb(EventConfig_t *eventConfig)
{
    zigbee_uart_t *uart = (zigbee_uart_t *)eventConfig->tag;
    int rcount = 0;
    char *buf = eventConfig->buf;
    int uartfd = eventConfig->fd;
    rcount = Read_unblock(uartfd, buf, BUF_LEN);
    if(rcount == -1)    goto err;
    int i;
#ifdef DEBUG
    printf("\n--------------------------------------\n");
    write(STDIN_FILENO, buf, rcount);
    printf("\n--------------------------------------\n");
    printf("--uart--buflen: %d\n", rcount);
    for(i = 0; i < rcount; i++)
    {
        printf("%02X ", (unsigned char)buf[i]);
    }
    printf("\n");
#endif //DEBUG
    getCompleteFrame((uint8_t *)buf, rcount, uart);
    if(uart->readStatus == READ_FINSH) //一个完整的包
    {
        buf = (char *)uart->outbuf;
#ifdef DEBUG
        printf("------pack----buflen: %d\n", uart->destlen);
        for(i = 0; i < uart->destlen; i++)
        {
            printf("%02X ", (unsigned char)buf[i]);
        }
        printf("\n");
#endif //DEBUG
        buf += strlen(DEV_MAGIC);
        //接受地址
        uint16_t addr = *(uint16_t *)buf;
        buf += sizeof(uint16_t) * 2;
        //校验码不准确
        if(uart_chkSum((uint8_t *)buf, uart->len) != buf[uart->len]) 
        {
            loge("check error.\n");
            return 0;
        }
        zigbee_dev_t *dev = seachOneByRequired(zigbeeDevList_head, 
                    (required_callback)zigbee_isAddr, &addr);
        EventConfig_t *evt = NULL;
        if(dev == NULL) //第一次连接
        {
#if 1
            char buf2[BUF_LEN];
            dev_dePackage(buf, uart->len, buf2, BUF_LEN);
            logd("%s\n", buf2);
            zigbee_dev_t *tmp;
            //已经有客户端
            if((tmp = seachOneByRequired(zigbeeDevList_head, 
               (required_callback)zigbee_isId, buf2 + 3))) 
            {
                logd("---------1--2--\n");
                if(tmp->addr != addr)
                {
                    logd("-----------!!!!------\n");
                    eventDel(tmp->ep_event);
                    deleteNode(&zigbeeDevList_head, 
                            (required_callback)zigbee_isAddr, &tmp->addr, free);
                }
            }
#endif //1
            dev = malloc(sizeof(zigbee_dev_t));
            if(!dev)    return 0;
            dev->addr = addr;
            memcpy(dev->id, buf2 + 3, ID_LEN);
            char ip[INET_ADDRSTRLEN];
            if(getGateWay(AF_INET, ip) == NULL) return 0;
            int connfd = connServFd(DOMAIN, ip, SERVER_PORT);
            if(connfd == -1)
            {
                free(dev);
                return 0;
            }
            evt = eventInit(eventConfig->epfd, connfd, EPOLLOUT, 
                            recvServer_cb, sendClient_cb, CLIENT_SOCKET, DOMAIN);
            if(evt == NULL || eventAdd(evt))
            {
                free(dev);
                return 0;
            }
            appendTailList(&zigbeeDevList_head, dev);
            dev->ep_event = evt;
            evt->tag = dev;
        } 
        else
        {
            evt = dev->ep_event;
            _switchEventMode(evt, EPOLLOUT);
        }
        memcpy(evt->buf, buf, uart->len);
        evt->buflen = uart->len;
    }
    return 0;
err:
    logd("uart close.\n");
    return -1;
}

int recvServer_cb(EventConfig_t *eventConfig)
{
    zigbee_dev_t *dev = (zigbee_dev_t *)eventConfig->tag;
    int rcount = 0;
    char *buf = eventConfig->buf;
    rcount = Read_unblock(eventConfig->fd, buf, BUF_LEN);
    if(rcount == -1)    goto err;
    buf[rcount] = '\0';  //保证数据正确
    EventConfig_t *uartEvent = seachOneByIdxEventList(UART_FD, 0);
    if(uartEvent == NULL)   goto err;
    uint32_t len = makeHeader(uartEvent->buf, dev->addr, rcount);
    memcpy(uartEvent->buf + len, buf, rcount);
    uint8_t chkSum = uart_chkSum((uint8_t *)buf, rcount);
    uartEvent->buflen = len + rcount;
    uartEvent->buf[uartEvent->buflen++] = chkSum;
    _switchEventMode(uartEvent, EPOLLOUT);
    return 0;
err:
    deleteNode(&zigbeeDevList_head, 
              (required_callback)zigbee_isAddr, &dev->addr, free);
    eventDel(eventConfig);
    return 0;
}

int sendClient_cb(EventConfig_t *connEvent)
{
    int ret;
    char *buf = connEvent->buf;
#ifdef DEBUG
    printf("----send----buflen: %d\n", connEvent->buflen);
        for(uint16_t i = 0; i < connEvent->buflen; i++)
        {
            printf("%02X ", (unsigned char)connEvent->buf[i]);
        }
    printf("\n");
#endif //DEBUG
    ret = Write(connEvent->fd, buf, connEvent->buflen); //回写客户端
    if(ret <= 0)
    {
        logd("write err.\n");
    }
    //将事件切换成读事件
    _switchEventMode(connEvent, EPOLLIN);
    return 0;
}