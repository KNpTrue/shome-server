/**
 * event-callback.h
 * 
 * |---2---|---data_len---|
 *   addr        
*/
#ifndef _EVENT_CALLBACK_H
#define _EVENT_CALLBACK_H

#include "../../src/event/loop.h"

//CC2530模块数据的处理
int recvUart_cb(EventConfig_t *eventConfig);
//socket客户端与服务端之间的通信
int recvServer_cb(EventConfig_t *eventConfig);

/**
 * 发送数据给客户端
*/
int sendClient_cb(EventConfig_t *connEvent);

#endif //_EVENT_CALLBACK_H