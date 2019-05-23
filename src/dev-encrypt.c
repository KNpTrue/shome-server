/**
 * @file dev-encrypt.c
*/
#include "dev-encrypt.h"
#include "random.h"

#include <string.h>

#define  MASKEY_LEN  4

uint32_t dev_enPackage(const char *src, uint32_t srclen, 
                        char *dest, uint32_t destlen_max, rand_callback_t cb)
{
    if(src == NULL || dest == NULL || srclen + MASKEY_LEN > destlen_max)
        return 0; 
    //使用掩码
    uint8_t maskey[MASKEY_LEN] = {0};
    getRandomString(maskey, MASKEY_LEN, cb); //获得掩码
    memcpy(dest, maskey, MASKEY_LEN);
    dest += MASKEY_LEN;
    for(uint32_t i = 0; i < srclen; i++)
    { 
        *dest++ = src[i] ^ maskey[i % 4];
    }
    return srclen + MASKEY_LEN;
}

uint32_t dev_dePackage(const char *src, uint32_t srclen, 
                        char *dest, uint32_t destlen_max)
{
    if(src == NULL || dest == NULL || srclen - MASKEY_LEN > destlen_max)
        return 0;
    //掩码
    uint8_t maskey[MASKEY_LEN];
    uint32_t i;
    for(i = 0; i < MASKEY_LEN; i++)
    {
        maskey[i] = *src++;
    }
    srclen -= MASKEY_LEN;
    //解数据
    for(i = 0; i < srclen; i++)
    {
        *dest++ = src[i] ^ maskey[i % 4];
    }
    return srclen; 
}
