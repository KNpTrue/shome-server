//event-loop.c
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "event-loop.h"
#include "../warp.h"
/**
 * 将所有的eventConfig结构体追加到hash管理的链表中以便管理, 销毁
*/
#define HASH_MAX 3
enum listIdx{
    HASH_LISTEN,
    HASH_WEBCONN,
    HASH_DEVCONN,
};
uint32_t getHash(uint8_t who); //获得哈希值
static node_t *eventlist_head[HASH_MAX] = {NULL};
//epoll_wait返回监听事件时的buf
static struct epoll_event retEvents[EPOLL_MAX_LISTEN];
//回调函数 根据fd删除结点
bool isEventFd_cb(void *data, void *tag);
//回调函数 关闭描述符
void destoryEvtConfig_cb(void *data);

EventConfig_t *initEvent(int epfd, int fd, uint32_t event, 
callback_t callback, uint8_t who, int domain)
{
   if(domain != AF_INET && domain != AF_INET6)
    {
#ifdef DEBUG_EVENT
        fprintf(stderr, "<initEvent>domain error.\n");
#endif //DEBUG_EVENT
        return NULL;
    }
    EventConfig_t *eventConfig = (EventConfig_t *)malloc(sizeof(EventConfig_t));
    if(eventConfig == NULL)
    {
#ifdef DEBUG_EVENT
        fprintf(stderr, "<initEvent>malloc error.\n");
#endif //DEBUG_EVENT
        return NULL;
    }
    eventConfig->epfd = epfd;
    eventConfig->fd = fd;
    eventConfig->event = event;
    eventConfig->callback = callback;
    eventConfig->registered = false;
    eventConfig->who = who;
    eventConfig->domain = domain;
    eventConfig->buflen = 0;
    eventConfig->tag = NULL;
    memset(eventConfig->buf, 0, BUF_LEN);
    return eventConfig;
}

int loopInit()
{
    int epoll_fd = epoll_create(EPOLL_MAX_LISTEN);
    if(epoll_fd == -1)
    {
#ifdef DEBUG_EVENT
        perror("<loopInit>epoll_create err");
#endif //DEBUG_EVENT
        return -1;
    }
    return epoll_fd;
}

int eventAdd(EventConfig_t *event)
{
    if(event->registered == true)  //如果已经挂在树上了就修改他
        return eventMod(event);
    //追加事件到链表中
    appendList(&eventlist_head[getHash(event->who)], event);
    struct epoll_event epevt;
    epevt.data.ptr = (void *)event;
    epevt.events = event->event;
    if(epoll_ctl(event->epfd, EPOLL_CTL_ADD, event->fd, &epevt)) //如果返回-1
    {
#ifdef DEBUG_EVENT
        perror("<eventAdd>epoll_ctl err");
#endif //DEBUG_EVENT
        return -1;
    }
    event->registered = true;
    return 0;
}

int eventDel(EventConfig_t *event)
{
    struct epoll_event epevt;
    epevt.data.ptr = (void *)event;
    epevt.events = event->event;
    if(epoll_ctl(event->epfd, EPOLL_CTL_DEL, event->fd, &epevt)) //如果返回-1
    {
#ifdef DEBUG_EVENT
        perror("<eventDel>epoll_ctl err");
#endif //DEBUG_EVENT
        return -1;
    }
    //将事件从链表中移除
    deleteNode(&eventlist_head[getHash(event->who)], 
    isEventFd_cb, &event->fd, destoryEvtConfig_cb);
    return 0;
}

int eventMod(EventConfig_t *event)
{
    if(event->registered == false)  //如果已经挂在树上了就修改他
        return eventAdd(event);
    //追加事件到链表中
    struct epoll_event epevt;
    epevt.data.ptr = (void *)event;
    epevt.events = event->event;
    if(epoll_ctl(event->epfd, EPOLL_CTL_MOD, event->fd, &epevt)) //如果返回-1
    {
#ifdef DEBUG_EVENT
        perror("<eventAdd>epoll_ctl err");
#endif //DEBUG_EVENT
        return -1;
    }
    return 0;
}

int loop(int epfd)
{
    int waitNums; //epoll_wait返回的个数
    int i;
    while(1)
    {
        waitNums = epoll_wait(epfd, retEvents, EPOLL_MAX_LISTEN, -1);
        if(waitNums == -1)
        {
            if(errno == EINTR)  continue;
#ifdef DEBUG_EVENT
            perror("<loop>epoll_wait err");
#endif //DEBUG_EVENT
            return -1;
        }
        EventConfig_t *eventConfig;
        for(i = 0; i < waitNums; i++)
        {
            eventConfig = (EventConfig_t *)retEvents[i].data.ptr;
            eventConfig->callback(eventConfig);
        }
    }
    return 0;
}

void loopDone(int epfd)
{
    Close(epfd);
    //删除链表
    int i;
    for(i = 0; i < HASH_MAX; i++)
        deleteList(&eventlist_head[i], destoryEvtConfig_cb);
#ifdef DEBUG_EVENT
    printf("loop done.\n");
#endif //DEBUG_EVENT
}

void travelEventList(uint8_t who, manipulate_callback manipulate, void *tag)
{
    travelList(eventlist_head[getHash(who)], manipulate, tag);
}

void *seachOneEventList(uint8_t who, required_callback required, void *tag)
{
    return seachOneByRequired(eventlist_head[getHash(who)], required, tag);
}

bool isEmptyEventList(uint8_t who)
{
    return isEmptyList(eventlist_head[getHash(who)]);
}

/******eventlist method*****/
uint32_t getHash(uint8_t who)
{
    if((who & DEVICE_LISTENER) || (who & WEBSOCKET_LISTENER)) //listen
        return HASH_LISTEN;
    else if(who & WEBSOCKET_CONNECTOR)
        return HASH_WEBCONN;
    else if(who & DEVICE_CONNECTOR)
        return HASH_DEVCONN;
    return 0;
}

bool isEventFd_cb(void *data, void *tag)
{
    EventConfig_t *evt = (EventConfig_t *)data;
    if(evt->fd == *((int *)tag))
        return true;
    return false;
}

void destoryEvtConfig_cb(void *data)
{
    EventConfig_t *eventConfig = (EventConfig_t *)data;
    Close(eventConfig->fd);
#ifdef DEBUG_EVENT
    printf("fd: %d close\n", eventConfig->fd);
#endif
    free(eventConfig);
}