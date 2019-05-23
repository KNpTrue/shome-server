//common.h
#ifndef _COMMON_H
#define _COMMON_H

#include <stdint.h>

#include "../src/warp.h"
#include "../src/key.h"
#include "../src/list.h"
#include "../src/dev-type.h"
#include "../src/dev-encrypt.h"

#define DEV_SWITCH

#define     DEV_MAGIC   "DEV"
#define     ID_LEN      8

//protocol head code
#define DEV_PRO_UPDATE  0x12
#define DEV_PRO_SET     0x41
#define DEV_PRO_RST     0x33

//获得网关地址
char *getGateWay(int domain, char *gateway);
//连接服务器并返回一个文件描述符
int connServFd(int domain, const char *addr, short port);

/**
 * @brief 握手 -- 发送
 * @param buf - 准备的数据包缓冲区
 * @param buflen - 缓冲区长度
 * @param id - 设备的ID
 * @param type - 设备类型
 * @param keylist_head 属性链表的头指针
 */
uint32_t handShake_send(char *buf, uint32_t buflen, uint8_t id[ID_LEN], 
                dev_type_t type, node_t *keylist_head);

/**
 * @brief 握手 -- 接受服务端发过来的标志位进行判断
 * @param buf 服务端发送给设备的数据
 * @return 对标志位进行检查的结果
 * @retval 0 正确
 * @retval -1 错误
 */
int handShake_recv(const char *buf);

int handShake(int fd, uint8_t id[ID_LEN], dev_type_t type, node_t *keylist_head);

//update
uint32_t dev_update(char *buf, node_t *keylist_head);

//key return head
node_t *initKeyList(char *keyName[], uint8_t *keyType, uint8_t *keyMode, char *keyUnit[]);

#endif //_COMMON_H