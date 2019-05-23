/**
 * @file dev-protocol.h
 * 
 * 握手协议
 * 固定：
 * |---3---|-ID_LEN-|-sizeof(dev_type_t)-|---1---|
 * dev_magic   id          type           keyNum 
 * 不固定(长度由keyNum决定)：
 * |-KEY_LEN-|---1---|---1---|-UNIT_LEN-|...
 *   keyname  keytype keymode  keyunit   ...
 * 
 * update Data
 * |---1---|---1---|--sizeof(keytype)--|...
 *    0x12   keyNum        keyvalue  ...
 * 
 * set Data
 * |---1---|--KEY_LEN--|--size for keyType--|
 *   0x41     keyName          keyValue
 * 
 * reset
 * |---1---|
 *   0x33
*/
#ifndef _DEVSOCKET_H
#define _DEVSOCKET_H

#include <stdint.h>
#include <stdbool.h>

#include "dev.h"
#include "../dev-encrypt.h"

#define DEV_MAGIC "DEV"

//protocol head code
#define DEV_PRO_UPDATE  0x12
#define DEV_PRO_SET     0x41
#define DEV_PRO_RST     0x33

//初始化设备
DevConfig_t *dev_handShake(const char *buf, uint32_t buflen);
//更新设备信息
int dev_getData(const char *buf, uint32_t buflen, DevConfig_t *devConfig);
//制作数据
uint32_t dev_makeData(char *buf, _key_t *key);

#endif //_DEVSOCKET_H