/**
 * base64.h
 * 来源于互联网
 * 编码字符串为标准字符串
*/
#ifndef _BASE64_H_
#define _BASE64_H_

#include <string.h>
 
static const char *ALPHA_BASE = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
 //编码
static char *
base64_encode(const char *buf, unsigned int size, char *base64Char, unsigned int bash64CharSize) {
    if(buf == NULL || size == 0)    return NULL;
    memset(base64Char, 0, bash64CharSize);
    int a = 0;
    int i = 0;
    while (i < size) {
        char b0 = buf[i++];
        char b1 = (i < size) ? buf[i++] : 0;
        char b2 = (i < size) ? buf[i++] : 0;
         
        int int63 = 0x3F; //  00111111
        int int255 = 0xFF; // 11111111
        base64Char[a++] = ALPHA_BASE[(b0 >> 2) & int63];
        base64Char[a++] = ALPHA_BASE[((b0 << 4) | ((b1 & int255) >> 4)) & int63];
        base64Char[a++] = ALPHA_BASE[((b1 << 2) | ((b2 & int255) >> 6)) & int63];
        base64Char[a++] = ALPHA_BASE[b2 & int63];
    }
    switch (size % 3) {
        case 1:
            base64Char[--a] = '=';
        case 2:
            base64Char[--a] = '=';
    }
    return base64Char;
}
 //解码
static char *
base64_decode(const char *base64Char, unsigned int base64CharSize, char *originChar, unsigned int originCharSize) {
    if(base64Char == NULL || base64CharSize == 0 || originChar == NULL)     return NULL;
    memset(originChar, 0, originCharSize);
    int toInt[128] = {-1}, i;
    for (i = 0; i < 64; i++) {
        toInt[ALPHA_BASE[i]] = i;
    }
    int int255 = 0xFF;
    int index = 0;
    for (i = 0; i < base64CharSize; i += 4) {
        int c0 = toInt[base64Char[i]];
        int c1 = toInt[base64Char[i + 1]];
        originChar[index++] = (((c0 << 2) | (c1 >> 4)) & int255);
        if (index >= originCharSize) {
            return originChar;
        }
        int c2 = toInt[base64Char[i + 2]];
        originChar[index++] = (((c1 << 4) | (c2 >> 2)) & int255);
        if (index >= originCharSize) {
            return originChar;
        }
        int c3 = toInt[base64Char[i + 3]];
        originChar[index++] = (((c2 << 6) | c3) & int255);
    }
    return originChar;
}

#endif