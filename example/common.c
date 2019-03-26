//common.c
#include "common.h"

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

void getRandomString(uint8_t *str, uint32_t strlen);

char *getGateWay(int domain, char *gateway)
{
    if(domain != AF_INET && domain != AF_INET6)
    {
#ifdef DEBUG_DEV
        fprintf(stderr, "<getGateWay>domain error.\n");
#endif //DEBUG_DEV
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
#ifdef DEBUG_DEV
        fprintf(stderr, "<connServFd>domain error.\n");
#endif //DEBUG_DEV
        return -1;
    }
    int fd = socket(domain, SOCK_STREAM, 0);
    if(fd == -1)
    {
#ifdef DEBUG_DEV
        perror("<connServFd>socket err");
#endif //DEBUG_DEV
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
#ifdef DEBUG_DEV
        perror("<connServFd>connect err");
#endif //DEBUG_DEV
        return -1;
    }
#ifdef DEBUG_DEV
    printf("connect success\n");
#endif //DEBUG_DEV
    return fd;
}

int handShake(int fd, uint8_t id[ID_LEN], dev_type_t type, node_t *keylist_head)
{
    //握手
    char buf[BUF_LEN], buf2[BUF_LEN];
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
    uint32_t len = enPackage(buf, p - buf, buf2, BUF_LEN);
    if(len == 0)    return -1;
    int ret = Write(fd, buf2, len);
    if(ret <= 0)    return -1;
    ret = Read(fd, buf, BUF_LEN);
    if(ret <= 0)    return -1;
    len = dePackage(buf, ret, buf2, BUF_LEN);
    if(len == 0)    return -1;
    if(strncmp(buf2, DEV_MAGIC, len)) return -1;
    p = buf;
    travelList(keylist_head, (manipulate_callback)valueToBuf, &p);
    len = enPackage(buf, p - buf, buf2, BUF_LEN);
    if(len == 0)    return -1;
    ret = Write(fd, buf2, len);
    if(ret <= 0)    return -1;
    return 0;
}

uint32_t enPackage(const char *src, uint32_t srclen, char *dest, uint32_t destlen_max)
{
    if(src == NULL || dest == NULL || srclen + MASKEY_LEN > destlen_max)
        return 0; 
    //使用掩码
    uint8_t maskey[MASKEY_LEN] = {0};
    getRandomString(maskey, MASKEY_LEN); //获得掩码
    memcpy(dest, maskey, MASKEY_LEN);
    dest += MASKEY_LEN;
    //加密
    uint32_t i;
    for(i = 0; i < srclen; i++)
    {
        *dest++ = src[i] ^ maskey[i % 4];
    }
    return srclen + MASKEY_LEN;
}

uint32_t dePackage(const char *src, uint32_t srclen, char *dest, uint32_t destlen_max)
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

void getRandomString(uint8_t *str, uint32_t strlen)
{
    int i;
    for(i = 0; i < strlen; i++)
    {
        str[i] = (uint8_t)rand() % 256;
    }
}

node_t *initKeyList(char *keyName[], uint8_t *keyType, uint8_t *keyMode, char *keyUnit[])
{
    node_t *head = NULL;
    int i;
    for(i = 0; keyName[i]; i++)
    {
        if(!appendTailList(&head, initKey(keyName[i], keyType[i], keyMode[i], keyUnit[i])))
            deleteList(&head, free);
    }
    return head;
}