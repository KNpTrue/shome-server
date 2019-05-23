/**
 * zigbee-protocol.h
 * 
 * 所有串口数据全部遵循以下协议
 * |--3--|----2----|---2---|---BODY_LEN---|-----1-----|
 *  "DEV"  ZB_ADDR  BDOY_LEN     BODY      Body_chkSum
*/

#ifndef _ZIGBEE_PROTOCOL_H
#define _ZIGBEE_PROTOCOL_H

#include "../../src/shome-types.h"
#include "../common.h"

#define HEADER_LEN 7
#define CHKSUM_LEN 1
#ifndef BUF_LEN
#define BUF_LEN 4096
#endif //BUF_LEN

#define PACKET_MAX 256

typedef struct zigbee_dev {
    char        id[ID_LEN];
    uint16_t    addr;
    void        *ep_event;
} zigbee_dev_t;

//readStatus
enum {
    READ_DEV, //"DEV"
    READ_ADDR, //ZIGBEE ADDR
    READ_LEN, //BODY LEN
    READ_BODY, //BODY
    READ_FINSH
};

typedef struct zigbee_uart {
    int         readStatus;
    uint8_t     outbuf[BUF_LEN];
    uint8_t     tmpbuf[BUF_LEN];
    uint16_t    destlen;
    uint16_t    tmplen;
    uint16_t    len;
} zigbee_uart_t;

/**
 * @brief 初始化串口设备
 * @param path 路径
 * @return fd 或 -1
 * @retval -1 失败
 * @retval >0 fd
 */
int uart_init(const char *path);

/**
 * @brief 初始化函数
 * @return zigbee_uart_t 结构体指针 malloc
*/
zigbee_uart_t *zigbee_uart_init();

/**
 * @brief uart 获取一帧
 * @param inbuf 
 * @param inCnt
 * @param uart
 */
void getCompleteFrame(uint8_t *inBuf, uint16_t inCnt, zigbee_uart_t *uart);

/**
 * @brief 打包头部
 * @param srcAddr -- 设备地址
 * @param datalen -- 设备发送的数据长度
 * @return 头部长度
 */
uint32_t makeHeader(char *buf, uint16_t srcAddr, uint16_t datalen);

//该设备地址是否为addr
bool zigbee_isAddr(zigbee_dev_t *dev, uint16_t *addr);

/**
 * @brief 算出校验码 将所有字节相加
 * @return 校验码
 */
uint8_t uart_chkSum(uint8_t *buf, uint16_t buflen);

//该设备地址是否为addr
bool zigbee_isAddr(zigbee_dev_t *dev, uint16_t *addr);
bool zigbee_isId(zigbee_dev_t *dev, char *id);

#endif //_ZIGBEE_PROTOCOL_H