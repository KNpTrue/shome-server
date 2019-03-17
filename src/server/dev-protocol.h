/**
 * dev-protocol.h
 * 握手协议
 * |----4-----|----3-----|---8---|---1---|---1---|-KEY_LEN-|---1---|---1---|-UNIT_LEN-|
 *    MASK      dev_magic   id     type    keyNum  keyname  keytype keymode  keyunit
 * 
 * update
 * DATA
 * |---1---|--sizeof(keytype)--|...
 *  keyNum        keyvalue  ...
 * 
 * makeData
 * |--KEY_LEN--|--size for keyType--|
 *    keyName          keyValue
*/
#ifndef _DEVSOCKET_H
#define _DEVSOCKET_H

#include <stdint.h>
#include <stdbool.h>

#include "dev.h"

#define DEV_MAGIC "DEV"



//初始化设备
DevConfig_t *dev_handShake(const char *buf, uint32_t buflen);
//更新设备信息
int dev_getData(const char *buf, uint32_t buflen, DevConfig_t *devConfig);
//
uint32_t dev_makeData(char *buf, _key_t *key);

//打包
uint32_t dev_enPackage(const char *src, uint32_t srclen, char *dest, uint32_t destlen_max);
//解包
uint32_t dev_dePackage(const char *src, uint32_t srclen, char *dest, uint32_t destlen_max);

#endif //_DEVSOCKET_H