#include <stdio.h>
#include <stdlib.h>

#include "../../src/event/loop.h"
#include "event-who.h"
#include "event-callback.h"
#include "zigbee-protocol.h"

#define   UART_DEV  "/dev/ttyS1"

static int epfd;

int main()
{
    //
    //list init
    list_malloc = malloc;
    list_free = free;

    //初始化uart
    int uart_fd = uart_init(UART_DEV);
    if(uart_fd == -1) return -1;
    //初始化epfd
    epfd = loopInit(HASH_MAX, getHash);
    if(epfd == -1)  return -2;
    //初始化事件
    EventConfig_t *uartEvent = eventInit(epfd, uart_fd, 
                        EPOLLIN, recvUart_cb, sendClient_cb, UART_FD, -1);
    if(uartEvent == NULL)   return -3;
    zigbee_uart_t *zigbee_uart = zigbee_uart_init();
    if(zigbee_uart == NULL) return -4;
    uartEvent->tag = zigbee_uart;
    //添加事件
    eventAdd(uartEvent);
    return loop(epfd);
}