/**
 * @file dev-encrypt.h
 * 设备加密算法
*/
#ifndef __DEV_ENCRYPT_H
#define __DEV_ENCRYPT_H

#include "shome-types.h"

/**
 * @brief 打包
 * @param src 要打包的数据
 * @param srclen 打包数据的长度
 * @param dest 目标的缓冲区
 * @param destlen_max 目标缓冲区的大小
 * @return 长度
 * @retval >0 success
 * @retval 0 fail
 */
uint32_t dev_enPackage(const char *src, uint32_t srclen, 
                        char *dest, uint32_t destlen_max, rand_callback_t cb);

/**
 * @brief 解包
 * @param src 要解包的数据
 * @param srclen 要解包数据的长度
 * @param dest 目标的缓冲区
 * @param destlen_max 目标缓冲区的大小
 * @return 长度
 * @retval >0 success
 * @retval 0 fail
 */
uint32_t dev_dePackage(const char *src, uint32_t srclen, char *dest, uint32_t destlen_max);

#endif //__DEV_ENCRYPT_H