/**
 * event-who.h
*/
#ifndef _EVENT_WHO_H
#define _EVENT_WHO_H
#include <stdint.h>

//who
#define    WEBSOCKET_LISTENER     (1<<0)   //websocket listen的socket
#define    WEBSOCKET_CONNECTOR    (1<<1)   //webscoket connect的socket
#define    WEBSOCKET_FIRSTCONN    (1<<2)   //第一次连接websocket
#define    DEVICE_LISTENER        (1<<3)   //设备监听
#define    DEVICE_CONNECTOR       (1<<4)   //默认设备
#define    DEVICE_FIRSTCONN       (1<<5)   //第一次连接注册设备

#define HASH_MAX 3

enum listIdx{
    HASH_LISTEN,
    HASH_WEBCONN,
    HASH_DEVCONN,
};

/******eventlist method*****/
static uint32_t getHash(uint8_t who)
{
    if((who & DEVICE_LISTENER) || (who & WEBSOCKET_LISTENER)) //listen
        return HASH_LISTEN;
    else if(who & WEBSOCKET_CONNECTOR)
        return HASH_WEBCONN;
    else if(who & DEVICE_CONNECTOR)
        return HASH_DEVCONN;
    return 0;
}

#endif //_EVENT_WHO_H