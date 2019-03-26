//common.h
#ifndef _COMMON_H
#define _COMMON_H

#include <stdint.h>

#include "../src/warp.h"
#include "../src/key.h"
#include "../src/list.h"
#include "../src/dev-type.h"

#define DEV_SWITCH

#define   DEV_MAGIC    "DEV"
#define   ID_LEN      8

//flag
#define DEV_OFF         0
#define DEV_ON          1<<0
#define DEV_AUTOUPDATE  1<<1

//获得网关地址
char *getGateWay(int domain, char *gateway);
//连接服务器并返回一个文件描述符
int connServFd(int domain, const char *addr, short port);
//握手
int handShake(int fd, uint8_t id[ID_LEN], dev_type_t type, node_t *keylist_head);
//打包
uint32_t enPackage(const char *src, uint32_t srclen, char *dest, uint32_t destlen_max);
//解包
uint32_t dePackage(const char *src, uint32_t srclen, char *dest, uint32_t destlen_max);

//key return head
node_t *initKeyList(char *keyName[], uint8_t *keyType, uint8_t *keyMode, char *keyUnit[]);

#endif //_COMMON_H