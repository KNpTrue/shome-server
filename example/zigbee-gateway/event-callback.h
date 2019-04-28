/**
 * event-callback.h
*/
#ifndef _EVENT_CALLBACK_H
#define _EVENT_CALLBACK_H

#include "../../src/event/loop.h"

//CC2530模块数据的处理
int sendUart_cb(EventConfig_t *eventConfig);
int recvUart_cb(EventConfig_t *eventConfig);
//socket客户端与服务端之间的通信
int sendServer_cb(EventConfig_t *eventConfig);
int recvServer_cb(EventConfig_t *eventConfig);

#endif //_EVENT_CALLBACK_H