/**
 * web-procotol.h
 * 
 * websocket协议相关
 * 第一次握手时利用子协议上的字段sec-websocket-protocol进行认证,该字段读取客户端的url，判断其是不是从特定站点访问该服务器
 * 
*/
#ifndef _WEBSOCKET_H
#define _WEBSOCKET_H

#include <stdbool.h>
#include <stdint.h>

#define  WEBSOCKET_KEY_LEN  64
#define  KEY_BUF_LEN  256
/************Opcode**************/
#define  OP_CONTINUE  0x00     //未知长度， 还会继续传输
#define  OP_TXTDATA   0x01     //文本文件
#define  OP_BINDATA   0x02     //二进制文件
#define  OP_DISCONN   0x08     //关闭连接
#define  OP_PING      0x09     //ping
#define  OP_PONG      0x0A     //pong
#define  OP_ERR       -1       //错误
//打包时候所需的选项
struct packageOpt{
    bool fin;
    uint8_t opcode;
    bool mask;
};
/**
 * 函数指针， 用于recvClient_cb, 处理数据
 * @opCode: 数据类型， 可改变
 * @isSwitchWrite: 是否自动切换位写事件
 * @return: 0 没有数据返回 >0 有数据返回
*/
typedef uint32_t (*webDealData_cb)(int *opCode, const char *src, 
uint32_t srclen, char *dest, uint32_t destlen_max);
/**
 * 从握手包中获得websocketkey
 * @buf:    要获取key的缓冲区
 * @key:    要存入key的缓冲区
 * @keylen: key的长度
 * @return: 成功返回true, 失败返回false
*/
bool getWebSocketValue(const char *buf, char *dest, uint32_t destlen_max, const char *flag);
/**
 * 将websocket-key 编码形成accpet-key
 * @src:     要获取key的缓冲区
 * @dest:    要存入key的缓冲区
 * @destlen: destkey的长度
 * @return:  成功返回true, 失败返回false
*/
bool encodeWebSocketKey(const char *src, char *dest, uint32_t destlen);
/**
 * 将回应头写入buf
 * @acceptkey: websocket accept-key
 * @subKey: websocket-protocol 
*/
void repondHeadBuild(char *buf, const char *acceptkey, const char *subKey);
/**
 * websocket解包(buf必须使用unsigned char里面设计到加法运算
 * 用char可能会引入负号导致长度计算不准确)
 * @package:     要解的包
 * @packagelen:  包的大小
 * @data:        解包存放数据的buf
 * @datalen_max: buf的长度
 * @datalen:     解包后数据长度
*/
int dePackage(const uint8_t *package, uint32_t packagelen, 
uint8_t *data, uint32_t datalen_max, uint32_t *datalen);
/**
 * websocket打包
 * @opcode:  OpCode值
 * @mask:    是否有掩码
 * @data:    原始数据
 * @datalen: 原始数据的长度
 * @package: 打包存放的buf
 * @package: buf的大小
*/
uint32_t enPackage(int opcode, bool mask, const uint8_t *data, 
uint32_t datalen, uint8_t *package, uint32_t packagelen_max);
/**
 * 用于recvWebClient_cb, 处理数据
 * @opCode: 数据类型， 可改变
 * @isSwitchWrite: 是否自动切换位写事件
 * @return: 0 没有数据返回 >0 有数据返回
*/
uint32_t webDealData(int *opCode, const char *src, 
uint32_t srclen, char *dest, uint32_t destlen_max);
/**
 * 对客户端传来的key进行认证
*/
bool isSubKeyRight(const char *subkey);

#endif //_WEBSOCKET_H