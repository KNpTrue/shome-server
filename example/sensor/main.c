//client.c
#include <stdio.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>

#include "../common.h"

#define   BUF_LEN      512
#define   DEVICE_ID    "YCadwFIlk"
#define   SERV_PORT    8003
#define   SERV_ADDR    "127.0.0.1"
//重写该函数
void updateKeyList(node_t *keylist_head);

int main()
{
    char ip[30];
    if(getGateWay(AF_INET, ip))
        printf("default gateway:%s\n", ip);
    int fd = connServFd(AF_INET, SERV_ADDR, SERV_PORT);
    if(fd == -1)    return -1;
    //握手
    char *keyName[] = {"温度", "湿度",NULL};
    uint8_t keyType[] = {KEY_NUMBER, KEY_NUMBER};
    uint8_t keyMode[] = {KEY_READONLY, KEY_READONLY};
    char *keyUnit[] = {"℃", "%RH"};
    node_t *keylist_head = initKeyList(keyName, keyType, keyMode, keyUnit);
    if(keylist_head == NULL)    goto err;
    if(!handShake(fd, (uint8_t *)DEVICE_ID, SM_TEMPL, keylist_head))
        printf("握手成功.\n");
    else goto err;
    char buf[BUF_LEN];
    char buf2[BUF_LEN];
    char *p;
    _key_t *key;
    ssize_t len;
    int ret;
    while(1)
    {
        updateKeyList(keylist_head);
        travelList(keylist_head, (manipulate_callback)printKey, NULL);
        //回写
        memset(buf2,  0, BUF_LEN);
        p = buf2;
        travelList(keylist_head, (manipulate_callback)valueToBuf, &p);
        len = enPackage(buf2, p - buf2, buf, BUF_LEN);
        if(len == 0)   goto err;
        if(Write(fd, buf, len) <= 0)    goto err;
        sleep(1);
    }
    return 0;
err:
    close(fd);
    return -1;
}

void updateKeyList(node_t *keylist_head)
{
    static double tmp = 1;
    tmp++;
    travelList(keylist_head, (manipulate_callback)setKeyValue, &tmp);
}