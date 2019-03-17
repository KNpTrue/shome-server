//client.c
#include <stdio.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>

#include "../common.h"

#define   BUF_LEN     512
#define   DEVICE_ID    "YCUCYYFI"
#define   SERV_PORT    8003
#define   SERV_ADDR    "127.0.0.1"

int main()
{
    char ip[30];
    if(getGateWay(AF_INET, ip))
        printf("default gateway:%s\n", ip);
    int fd = connServFd(AF_INET, SERV_ADDR, SERV_PORT);
    if(fd == -1)    return -1;
    //握手
    char *keyName[] = {"isOpen", "R", "G", "B", "bright", "1", "2", NULL};
    uint8_t keyType[] = {KEY_BOOL, KEY_RANGE, KEY_RANGE, KEY_RANGE, KEY_RANGE, KEY_BOOL, KEY_STRING};
    uint8_t keyMode[] = {KEY_READWRITE, KEY_READWRITE, KEY_READWRITE, KEY_READWRITE, KEY_READWRITE, KEY_READWRITE, KEY_READWRITE};
    char *keyUnit[] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL};
    node_t *keylist_head = initKeyList(keyName, keyType, keyMode, keyUnit);
    if(keylist_head == NULL)    goto err;
    if(!handShake(fd, (uint8_t *)DEVICE_ID, SM_LIGHT, keylist_head))
        printf("握手成功.\n");
    else goto err;
    char buf[BUF_LEN];
    char buf2[BUF_LEN];
    char *p;
    _key_t *key;
    ssize_t len;
    while(1)
    {
        len = Read(fd, buf, BUF_LEN);
        if(len <= 0)   return -1;
        dePackage(buf, len, buf2, BUF_LEN);
        p = buf2;
        key = seachOneByRequired(keylist_head, (required_callback)isSameKeyName, p);
        if(!key)    continue; 
        p += KEY_LEN;
        setKeyValue(key, (void *)p);
        //updateDev(key);
        printKey(key);
        //回写
        memset(buf2,  0, BUF_LEN);
        p = buf2;
        travelList(keylist_head, (manipulate_callback)valueToBuf, &p);
        len = enPackage(buf2, p - buf2, buf, BUF_LEN);
        if(Write(fd, buf, len) <= 0)    goto err;
    }
    return 0;
err:
    close(fd);
    return -1;
}