//main.c
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "../list.h"
#include "../event/loop.h"
#include "../log.h"
#include "event-who.h"
#include "event-socket.h"
#include "config.h"
#include "dev-time.h"
#include "web-config.h"
#include "web-pack.h"

//初始化守护进程
void initDaemon();
//尝试fork fail_sleep失败后多少秒后尝试
pid_t tryfork(int fail_sleep);
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
    //list init
    list_malloc = malloc;
    list_free = free;
    
    //initDaemon(); //初始化守护进程
    readConf(); //读取配置文件
    //获取webConfig
    WebConfig_t *webConfig = getWebConfig();
    initSignal();
    srand(time(NULL)); //设置随机种子
    //epoll_create
    epfd = loopInit(HASH_MAX, getHash); 
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
        webSocketEvent = eventInit(epfd, webSocketFd, EPOLLIN, 
        waitClient_cb, sendClient_cb, WEBSOCKET_LISTENER, domain[i]);
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
        deviceEvent = eventInit(epfd, deviceFd, EPOLLIN,
        waitClient_cb, sendClient_cb, DEVICE_LISTENER, domain[i]);
        if(deviceEvent == NULL) return -5;
        eventAdd(deviceEvent);
    }
    //从配置文件中读取todolist and devlist
    travelList(todolist_head, (manipulate_callback)registerTodo, NULL);
    travelList(setlist_head, (manipulate_callback)registerSet, NULL);
    travelList(roomlist_head, (manipulate_callback)registerRoom, NULL);
    setNextAlarm();
#ifdef DEBUG_DEV
    travelList(devlist_head, (manipulate_callback)printDev, NULL);
    travelList(todolist_head, (manipulate_callback)printTodo, NULL);
    travelList(setlist_head, (manipulate_callback)printSet, NULL);
    travelList(roomlist_head, (manipulate_callback)printRoom, NULL);
#endif //DEBUG_DEV
    return loop(epfd);
}

void initSignal()
{
    signal(SIGINT, sigint_cb);
    signal(SIGALRM, sigtime_cb);
}

void sigint_cb(int signo)
{
    writeConf();
    deleteList(&devlist_head, (destory_callback)destoryDevConfig);
    deleteList(&setlist_head, (destory_callback)destoryTaskSet);
    deleteList(&todolist_head, (destory_callback)destoryTodo);
    deleteList(&roomlist_head, (destory_callback)destoryRoom);
    resetAlarm();
    loopDone(epfd);
    
    exit(0);
}

void sigtime_cb(int signo)
{
    travelList(getNextSetList(), (manipulate_callback)runTaskSet, &epfd);
    resetAlarm();
    travelList(todolist_head, (manipulate_callback)updateNextAlarm, NULL);
    setNextAlarm();
}
//尝试fork fail_sleep失败后多少秒后尝试
pid_t tryfork(int fail_sleep);
void initDaemon()
{
    pid_t pid;
    pid = tryfork(1);
    //pid = fork();
    if(pid == -1)
    {
        logp("fork: ");
        exit(1);
    }
    else if(pid > 0)  //父进程
        exit(0);
    else if(pid == 0) // 子进程
    {
        setsid();//设置
        chdir("~");
        umask(0);
        //重定向到/dev/null
        int fd = open("/dev/null", O_RDWR);
        if(fd == -1)
        {
            logp("/dev/null: ");
            exit(1);
        }
        dup2(fd, STDIN_FILENO);
        dup2(fd, STDOUT_FILENO);
        dup2(fd, STDERR_FILENO);
        if(fd > 2)
            close(fd);
    }
}

pid_t tryfork(int fail_sleep)
{
    int tries = 0;
    pid_t pid;
    while(1)
    {
        pid = fork();
        if(pid == -1)
        {
            if(tries == 5)
                return -1;
            else
            {
                tries++;
                if(fail_sleep > 0)
                    sleep(fail_sleep);
                continue;
            }
        }
        else
            return pid;
    }
}