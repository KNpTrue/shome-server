/**
 * event-who.h
*/
#ifndef _EVENT_WHO_H
#define _EVENT_WHO_H

#include <stdint.h>
//who
#define   UART_FD   (1 << 0)
#define   CLIENT_SOCKET (1 << 1)
//hash num
#define   HASH_MAX  2
//hash idx
enum listIdx {
    HASH_UART,
    HASH_CLIENT
};

static uint32_t getHash(uint8_t who)
{
    if(who & UART_FD) //listen
        return HASH_UART;
    else if(who & CLIENT_SOCKET)
        return HASH_CLIENT;
    return 0;
}

#endif //_EVNET_WHO_H