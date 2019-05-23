//client.c
#include <stdio.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdlib.h>

#include "../common.h"

#define   BUF_LEN     512
#define   DEVICE_ID    "YCUCYYFI"
#define   SERV_PORT    8003
#define   SERV_ADDR    "127.0.0.1"

int main()
{
    //list init
    list_malloc = malloc;
    list_free = free;

    char ip[30];
    if(getGateWay(AF_INET, ip))
        printf("default gateway:%s\n", ip);
    int fd = connServFd(AF_INET, ip, SERV_PORT);
    if(fd == -1)    return -1;
    //握手
    char *keyName[] = {"isOpen", "R", "G", "B", "bright", NULL};
    uint8_t keyType[] = {KEY_BOOL, KEY_RANGE, KEY_RANGE, KEY_RANGE, KEY_RANGE};
    uint8_t keyMode[] = {KEY_READWRITE, KEY_READWRITE, KEY_READWRITE, KEY_READWRITE, KEY_READWRITE};
    char *keyUnit[] = {NULL, NULL, NULL, NULL, NULL};
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
        dev_dePackage(buf, len, buf2, BUF_LEN);
        if(*buf2 != DEV_PRO_SET) continue;
        p = buf2 + 1;
        key = seachOneByRequired(keylist_head, (required_callback)isSameKeyName, p);
        if(!key)    continue; 
        p += KEY_LEN;
        setKeyValue(key, (void *)p);
        //updateDev(key);
        printKey(key);
        //回写
        memset(buf2,  0, BUF_LEN);
        len = dev_update(buf2, keylist_head);
        len = dev_enPackage(buf2, len, buf, BUF_LEN, rand);
        if(Write(fd, buf, len) <= 0)    goto err;
    }
    return 0;
err:
    close(fd);
    return -1;
}