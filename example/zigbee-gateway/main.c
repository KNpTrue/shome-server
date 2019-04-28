#include <stdio.h>

#include "../../src/event/loop.h"
#include "event-who.h"

#define   UART_DEV  "/dev/tty0"

static int epfd; 

int main()
{
    int uart_fd = uart_init(UART_DEV);
    if(uart_fd == -1) return -1;

    epfd = loopInit(HASH_MAX, getHash);
    if(epfd == -1)  return -2;
    
    
    return 0;
}