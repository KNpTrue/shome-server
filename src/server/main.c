//main.c
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <signal.h>

#include "event-loop.h"
#include "event-socket.h"
#include "config.h"
#include "dev-time.h"
#include "web-config.h"
#include "web-pack.h"

//初始化信号
void initSignal();
//SIGINT处理函数 
void sigint_cb(int signo);
//SIGTIME处理函数
void sigtime_cb(int signo);
//epfd
static int epfd;

int main(int argc, char *argv[])
{
    readConf(); //读取配置文件
    //获取webConfig
    WebConfig_t *webConfig = getWebConfig();
    initSignal();
    srand(time(NULL)); //设置随机种子
    epfd = loopInit(); //epoll_create
    if(epfd == -1)  return -1;
    int domain[2] = {AF_INET, AF_INET6};
    int i;
    //设置websocket
    int web_port[2] = {webConfig->web_port, webConfig->web6_port};
    int webSocketFd;
    EventConfig_t *webSocketEvent;
    for(i = 0; i < 2; i++)
    {
        webSocketFd = initListenFd(domain[i], web_port[i]);
        if(webSocketFd == -1)   return -2;
        //将socket绑定到epoll监听树上
        webSocketEvent = initEvent(epfd, webSocketFd, EPOLLIN, 
        waitClient_cb, WEBSOCKET_LISTENER, domain[i]);
        webSocketEvent->tag = webDealData;
        if(webSocketEvent == NULL)  return -3;
        //添加websocket监听事件
        eventAdd(webSocketEvent);
    }
    //设置设备的监听
    int dev_port[2] = {webConfig->dev_port, webConfig->dev6_port};
    int deviceFd;
    EventConfig_t *deviceEvent;
    for(i = 0; i < 2; i++)
    {
        deviceFd = initListenFd(domain[i], dev_port[i]);
        if(deviceFd == -1)  return -4;
        //将socket绑定到epoll上
        deviceEvent = initEvent(epfd, deviceFd, EPOLLIN,
        waitClient_cb, DEVICE_LISTENER, domain[i]);
        if(deviceEvent == NULL) return -5;
        eventAdd(deviceEvent);
    }
    //从配置文件中读取todolist and devlist
    travelList(*getToDoListHead(), (manipulate_callback)registerTodo, NULL);
    travelList(*getSetListHead(), (manipulate_callback)registerSet, NULL);
    travelList(*getRoomListHead(), (manipulate_callback)registerRoom, NULL);
    setNextAlarm();
#ifdef DEBUG_DEV
    travelList(*getDevListHead(), (manipulate_callback)printDev, NULL);
    travelList(*getToDoListHead(), (manipulate_callback)printTodo, NULL);
    travelList(*getSetListHead(), (manipulate_callback)printSet, NULL);
    travelList(*getRoomListHead(), (manipulate_callback)printRoom, NULL);
#endif //DEBUG_DEV
    return loop(epfd);
}

void initSignal()
{
    struct sigaction int_act, time_act;
    int_act.sa_flags = 0;
    time_act.sa_flags = 0;
    int_act.sa_handler = sigint_cb;
    time_act.sa_handler = sigtime_cb;
    sigaction(SIGINT, &int_act, NULL);
    sigaction(SIGALRM, &time_act, NULL);
}

void sigint_cb(int signo)
{
    writeConf();
    deleteList(getDevListHead(), (destory_callback)destoryDevConfig);
    deleteList(getSetListHead(), (destory_callback)destoryTaskSet);
    deleteList(getToDoListHead(), (destory_callback)destoryTodo);
    deleteList(getRoomListHead(), (destory_callback)destoryRoom);
    resetAlarm();
    loopDone(epfd);
    
    exit(0);
}

void sigtime_cb(int signo)
{
    travelList(getNextSetList(), (manipulate_callback)runTaskSet, &epfd);
    resetAlarm();
    travelList(*getToDoListHead(), (manipulate_callback)updateNextAlarm, NULL);
    setNextAlarm();
}