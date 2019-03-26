/**
 * event-loop.h
 * 本服务器采用事件循环机制实现高并发, 通过linux中的系统函数epoll实现
 * 在一颗epoll监听树上绑定任意多的事件
 * 事件中描述了要操作的对象, 对对象操作的方法, 以及对象使用到的一些资源, 参考 EventConfig结构体
 * 每当监听树监听到有事件发生时, 就会自动调用事件的方法去执行相应的操作, 这个过程时循环发生的
 * 创建事件循环很简单, 按照下面4步进行(参考main.c):
 * 1.你需要先初始化事件循环 loopInit(), 它返回一个epoll监听描述符
 * 2.初始化每一个事件 initEvent(), 传入所有事件必要的信息
 * 3.将事件添加到事件循环中 eventAdd()
 * 4.开始循环 loop()
 */
#ifndef _EVENTLOOP_H
#define _EVENTLOOP_H

#include <sys/epoll.h>
#include <stdbool.h>
#include <arpa/inet.h>

#include "../list.h"

#define    EPOLL_MAX_LISTEN    32  //epoll监听描述符数量
#define    BUF_LEN             2048
//who
#define    WEBSOCKET_LISTENER     (1<<0)   //websocket listen的socket
#define    WEBSOCKET_CONNECTOR    (1<<1)   //webscoket connect的socket
#define    WEBSOCKET_FIRSTCONN    (1<<2)   //第一次连接websocket
#define    DEVICE_LISTENER        (1<<3)   //设备监听
#define    DEVICE_CONNECTOR       (1<<4)   //默认设备
#define    DEVICE_FIRSTCONN       (1<<5)   //第一次连接注册设备
//声明结构体
typedef struct EventConfig EventConfig_t;
//回调函数
typedef int (*callback_t)(EventConfig_t *eventConfig);
//事件结构体
struct EventConfig {
    int         epfd; //该事件所属epoll红黑树的监听描述符
    int         fd;  //该事件文件描述符
    int         domain; //AF_INET or AF_INET6
    uint32_t    event; //事件
    callback_t  callback; //回调函数指针
    void        *tag; //附加参数
    bool        registered; //是否已经在监听树上注册
    uint8_t     who;    //发送事件的对象是哪一类的
    char        buf[BUF_LEN];  //传输文件的buf
    uint32_t    buflen; //buflen
};
/**
 * 结构体event初始化
 * @epfd:     epoll文件描述符
 * @fd:       所要监听的文件描述符
 * @event:    epoll事件
 * @callback: 回调函数
 * @who:      发送事件的对象是哪一类的
 * @domain:   AF_INET or AF_INET6
 */
EventConfig_t *initEvent(int epfd, int fd, uint32_t event, 
callback_t callback, uint8_t who, int domain);
/**
 * 初始化循环
 * @返回值: epoll_fd
 */
int loopInit();
/**
 * 添加监听事件
 * @epfd:  epoll文件描述符
 * @event: 事件结构体
 */
int eventAdd(EventConfig_t *event);
/**
 *  删除监听事件
 *  @epfd:  epoll文件描述符
 *  @event: 事件结构体
 */
int eventDel(EventConfig_t *event);
/**
 * 修改监听事件
 * @epfd:  epoll文件描述符
 * @event: 事件结构体
 */
int eventMod(EventConfig_t *event);
/**
 * 开始循环
 * @epfd:   epoll监听描述符
 * @return: 0 success -1 err
 */
int loop(int epfd);
/**
 * 结束循环
 * @epfd: epoll监听描述符
*/
void loopDone(int epfd);
/**
 * 提供接口给外部去遍历事件链表, 基于list封装
 * @who: 事件的类型，具体参考上面定义的宏
 * @manipulate: 操作函数(回调)
 * @tag: 提供给操作函数的额外参数
 */
void travelEventList(uint8_t who, manipulate_callback manipulate, void *tag);
//提供接口给外部去查找事件链表
/**
 * 提供接口给外部去查找事件链表, 基于list封装
 * @who: 事件的类型，具体参考上面定义的宏
 * @required: 比较函数(回调)
 * @tag: 提供给操作函数的额外参数
 */
void *seachOneEventList(uint8_t who, required_callback required, void *tag);
/**
 * 检查对应类型的eventlist是否为空链表
 */
bool isEmptyEventList(uint8_t who);

#endif //_EVENTLOOP_H