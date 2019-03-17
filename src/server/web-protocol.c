//web-procotol.c
#include "web-protocol.h"
#include "web-config.h"
#include "web-pack.h"
#include "sha1-use.h"
#include "base64.h"
#include "random.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define  MAGIC_STRING  "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"

bool getWebSocketValue(const char *buf, char *dest, uint32_t destlen_max, const char *flag)
{
    if(buf == NULL || dest == NULL)  return false;
    char *keyBegin = NULL;
    keyBegin = strstr(buf, flag);  //获得初始位置
    if(!keyBegin)   return false;
    keyBegin += strlen(flag);
    for(; *keyBegin != '\r' && *keyBegin != '\n'; *dest++ = *keyBegin++) //拷贝key
    {
        if(--destlen_max == 1) //如果超出keylen范围, 最后一个字符填'\0'
            return false;
    }
    *dest = '\0';
    return true;
}

bool encodeWebSocketKey(const char *src, char *dest, uint32_t destlen)
{
    if(src == NULL || dest == NULL) return false;
    char buf[KEY_BUF_LEN];
    char sha1data[KEY_BUF_LEN];
    strcat(strcpy(buf, src), MAGIC_STRING); //key和magic string相连接
    if(!buf) return false;
    if(!SHA1_encode(buf, sha1data, KEY_BUF_LEN)) //sha-1编码
        return false; 
    if(!base64_encode(sha1data, SHA1HashSize, dest, destlen)) //bash64编码
        return false; 
    return true;
}

void repondHeadBuild(char *buf, const char *acceptkey, const char *protocol)
{
    const char httpDemo[] = "HTTP/1.1 101 Switching Protocols\r\n"
                                            "Upgrade: websocket\r\n"
                                            "Server: Microsoft-HTTPAPI/2.0\r\n"
                                            "Connection: Upgrade\r\n"
                                            "Sec-WebSocket-Accept: %s\r\n"
                                            "Sec-WebSocket-Protocol: %s\r\n"
                                            "\r\n";
    sprintf(buf, httpDemo, acceptkey, protocol);
}

int dePackage(const uint8_t *package, uint32_t packagelen, 
uint8_t *data, uint32_t datalen_max, uint32_t *datalen)
{
    int ret; //返回值
    char type = *package & 0x0F;
    //fin opcode
    if((*package & 0x80) == 0x80) //fin == 1 已知长度
    {
        switch(type)
        {
        case OP_BINDATA: ret = OP_BINDATA; break;
        case OP_TXTDATA: ret = OP_TXTDATA; break;
        case OP_DISCONN: ret = OP_DISCONN;break;
        case OP_PING:    ret = OP_PING;break;
        case OP_PONG:    ret = OP_PONG;break;
        default:         ret = OP_ERR; break;
        }
    }
    else //fin == 0 未知长度
    {
        if(type == OP_CONTINUE) //中间包
            ret = OP_CONTINUE;
        else    return OP_ERR;
    }
    package++; //偏移8bit
    //MASK
    bool mask = *package & 0x80;
    //PayLoad Len 0~125 or 126 or 127
    uint32_t len = *package++ & 0x7F;
    switch(len)
    {
    case 126: len = *package++; //读取接下来的16位
              len = (len << 8) + (uint8_t)*package++;
              if(packagelen < len + 4 + (mask == true ? 4 : 0)) //长度检查
                return OP_ERR;
              break;
    case 127: len = *package++; //读取接下来的64位
              int i = 3; while(i--){len = (len << 8) + *package++;}
              if(packagelen < len + 10 + (mask == true ? 4 : 0)) //长度检查
                return OP_ERR;
              break;
    }
    //data长度检查
    if(datalen_max <len)    return OP_ERR;
    *datalen = len;
    if(!mask) //不使用掩码
    {
        memcpy(data, package, len);
        data[len] = '\0';
        return ret;
    }
    //使用掩码
    uint8_t maskey[4];
    uint32_t i;
    memcpy(maskey, package, 4);
    package += 4; //偏移4个字节指向数据区
    for(i = 0; i < len; i++)
    {
        *data++ = package[i] ^ maskey[i % 4];
    }
    *data = '\0';
    return ret;
}

uint32_t enPackage(int opcode, bool mask, const uint8_t *data, 
uint32_t datalen, uint8_t *package, uint32_t packagelen_max)
{
    uint32_t packagelen = datalen + ((datalen < 65535 && datalen >125) ? 2 : 0) +
        (mask == true ? 4 : 0) + (datalen > 65535 && datalen < 0xFFFFFFFF ? 8 : 0) + 2;
    if(packagelen_max < packagelen) return 0;
    memset(package, 0, packagelen_max);
    
    if(opcode != 0x00)  *package = 0x80; //fin 置1
    *package++ |= opcode;
    
    //mask
    if(mask) *package = 0x80;
    if(datalen < 126)  *package++ |= datalen;
    else if(datalen < 65535)
    {
        *package++ |= 0x7E;
        *package++ |= ((datalen >> 8) & 0xFF);
        *package++ |= (datalen & 0xFF);
    }
    else if(datalen < 0xFFFFFFFF)
    {
        *package++ |= 0x7F;
        *package += 4;
        *package++ |= ((datalen >> 24) & 0xFF);
        *package++ |= ((datalen >> 16) & 0xFF);
        *package++ |= ((datalen >> 8) & 0xFF);
        *package++ |= (datalen & 0xFF);
    }
    if(!mask)
    {
        memcpy(package, data, datalen);
        return packagelen;
    }
    //使用掩码
    uint8_t maskey[4] = {0};
    getRandomString(maskey, 4); //获得掩码
    memcpy(package, maskey, 4);
    package += 4; //偏移4个字节指向数据区
    //加密数据
    char i;
    for(i = 0; i < datalen; i++)
    {
        *package++ = data[i] ^ maskey[i % 4];
    }
    return packagelen;
}

uint32_t webDealData(int *opCode, const char *src, 
uint32_t srclen, char *dest, uint32_t destlen_max)
{
    uint32_t ret = 0;
    switch(*opCode)
    {
    case OP_PING: //to pong
        *opCode = OP_PONG;
        if(destlen_max < srclen)    break;
        memcpy(dest, src, srclen);
        ret = srclen;
        break;
    case OP_PONG: //read
        break;
    case OP_TXTDATA: //文本
#ifdef DEBUG_WEB
        printf("-----------txt: %s\n", src);
#endif //DEBUG_WEB
        ret = json_analysis(src, dest, destlen_max);
        break;
    case OP_BINDATA: //data
#ifdef DEBUG_WEB
        printf("-----------data: %d\n", *src);
#endif //DEBUG_WEB
        
        break;
    }
    return ret;
}

bool isSubKeyRight(const char *subkey)
{
    if(!strcmp(getWebConfig()->passwd, subkey)) //key相等
        return true;
    return false;
}