//evnet-socket.h
#ifndef _EVENTSOCKET_H
#define _EVENTSOCKET_H

#include <arpa/inet.h>

#include <stdbool.h>
#include "../event/loop.h"
#include "web-protocol.h"
#include "dev-protocol.h"

#define  MAX_LISTEN  16

#define _switchEventMode(_struct, _event, _callback) \
        ({(_struct)->event = (_event);\
        (_struct)->callback = (_callback);\
        eventMod(_struct);})

/**
 * 初始化一个监听描述符
 * @domain: AF_INET or AF_INET6
 * @port:   端口
 * @return: 成功返回文件描述符  返回-1失败
*/
int initListenFd(int domain, short port);
/**
 * 连接客户端
*/
int waitClient_cb(EventConfig_t *listenEvent);
/**
 * 接受客户端的请求
*/
int recvClient_cb(EventConfig_t *connEvent);
/**
 * 发送数据给客户端
*/
int sendClient_cb(EventConfig_t *connEvent);
/**
 * 编码发送 websocket
 * type参考pack_type
 * tag 为数据
 */
void sendDataToWeb(uint8_t type, void *tag);

#endif //_EVENTSOCKET_H