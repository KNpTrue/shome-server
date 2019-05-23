//common.c
#include "common.h"
#include "../src/log.h"

#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>

#define  BUF_LEN     512
#define  MASKEY_LEN  4

//sockaddr
union sockaddr_types {
    struct sockaddr_in  in4;
    struct sockaddr_in6 in6;
};

char *getGateWay(int domain, char *gateway)
{
    if(domain != AF_INET && domain != AF_INET6)
    {
        loge("<getGateWay>domain error.\n");
        return NULL;
    }
    FILE *fp;
    char buf[512];  
    char cmd[128];  
    char *tmp;  
    if(domain == AF_INET)
        strcpy(cmd, "ip route");
    else 
        strcpy(cmd, "ip -6 route");
    fp = popen(cmd, "r");
    if(NULL == fp)
    {
        perror("popen error");
        return "";
    }
    int flag = 0;
    while(fgets(buf, sizeof(buf), fp) != NULL)
    {
        tmp =buf;
        while(*tmp && *tmp == ' ')
            ++ tmp;
        if(strncmp(tmp, "default", strlen("default")) == 0)
        {
            flag = 1;
            break;
        }
    }
    if(flag == 0)   return NULL;
    sscanf(buf, "%*s%*s%s", gateway);
    pclose(fp);

    return gateway;
}  

int connServFd(int domain, const char *addr, short port)
{
    if(domain != AF_INET && domain != AF_INET6)
    {
        loge("<connServFd>domain error.\n");
        return -1;
    }
    int fd = socket(domain, SOCK_STREAM, 0);
    if(fd == -1)
    {
        logp("<connServFd>socket err");
    }
    union sockaddr_types serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    if(domain == AF_INET)
    {
        serv_addr.in4.sin_family = AF_INET;
        serv_addr.in4.sin_port = htons(port);
        inet_pton(AF_INET, addr, &serv_addr.in4.sin_addr.s_addr);
    }
    else //AF_INET6
    {
        serv_addr.in6.sin6_family = AF_INET6;
        serv_addr.in6.sin6_port = htons(port);
        inet_pton(AF_INET6, addr, &serv_addr.in6.sin6_addr);
    }
    if(connect(fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)))
    {
        logp("<connServFd>connect err");
        return -1;
    }
    logd("connect success\n");
    return fd;
}

uint32_t handShake_send(char *buf, uint32_t buflen, uint8_t id[ID_LEN], 
                dev_type_t type, node_t *keylist_head)
{
    //握手
    memset(buf, 0, BUF_LEN);
    strcpy(buf, "DEV");
    char *p = buf + strlen("DEV");
    uint32_t  i;
    //id
    for(i = 0; i < ID_LEN; i++)
    {
        *p++ = id[i];
    }
    //type
    memcpy(p, &type, sizeof(dev_type_t));
    p += sizeof(dev_type_t);
    //key
    *p++ = getNodeCount(keylist_head);
    travelList(keylist_head, (manipulate_callback)copyKeyHead_move, &p);
    //打包加密
    return p - buf;
}

int handShake_recv(const char *buf)
{
    return strncmp(buf, DEV_MAGIC, sizeof(DEV_MAGIC)) == 0;
}

int handShake(int fd, uint8_t id[ID_LEN], dev_type_t type, node_t *keylist_head)
{
    char buf[BUF_LEN], buf2[BUF_LEN];
    uint32_t len = handShake_send(buf, BUF_LEN, id, type, keylist_head);
    if(len == 0)    return -1;
    len = dev_enPackage(buf, len, buf2, BUF_LEN, rand);
    if(len == 0)    return -1;
    int ret = Write(fd, buf2, len);
    if(ret <= 0)    return -1;
    ret = Read(fd, buf, BUF_LEN);
    if(ret <= 0)    return -1;
    len = dev_dePackage(buf, ret, buf2, BUF_LEN);
    if(len == 0)    return -1;
    if(handShake_recv(buf2))    return -1;
    len = dev_update(buf, keylist_head);
    if(len == 0)    return -1;
    len = dev_enPackage(buf, len, buf2, BUF_LEN, rand);
    if(len == 0)    return -1;
    ret = Write(fd, buf2, len);
    if(ret <= 0)    return -1;
    return 0;
}

uint32_t dev_update(char *buf, node_t *keylist_head)
{
    char *p = buf;
    *p++ = DEV_PRO_UPDATE;
    travelList(keylist_head, (manipulate_callback)valueToBuf, &p);
    return p - buf;
}

node_t *initKeyList(char *keyName[], uint8_t *keyType, uint8_t *keyMode, char *keyUnit[])
{
    node_t *head = NULL;
    int i;
    for(i = 0; keyName[i]; i++)
    {
        _key_t *key = malloc(sizeof(_key_t));
        if (key == NULL)    goto err;
        if(!initKey(key, keyName[i], keyType[i], keyMode[i], keyUnit[i]))
            goto err;
        if(!appendTailList(&head, key))
            goto err;
    }
    return head;
err:
    deleteList(&head, free);
    return NULL;
}