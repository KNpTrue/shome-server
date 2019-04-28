/**
 * @file loop.h
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
/* @def epoll监听描述符数量 */
#define    EPOLL_MAX_LISTEN    32
#define    BUF_LEN             2048
//声明结构体
typedef struct EventConfig EventConfig_t;
//回调函数
typedef int (*callback_t)(EventConfig_t *eventConfig);
typedef uint32_t (*getHash_t)(uint8_t who);
/**
 * @brief 事件结构体
 */
struct EventConfig {
    int         epfd; ///<该事件所属epoll红黑树的监听描述符
    int         fd;  ///<该事件文件描述符
    int         domain; ///<AF_INET or AF_INET6
    uint32_t    event; ///<事件
    callback_t  callback; ///<回调函数指针
    void        *tag; ///<附加参数
    bool        registered; ///<是否已经在监听树上注册
    uint8_t     who;    ///<发送事件的对象是哪一类的
    char        buf[BUF_LEN];  ///<传输文件的buf
    uint32_t    buflen; ///<buflen
};
/**
 * @brief 创建事件结构体
 * @param epfd      epoll文件描述符
 * @param fd        所要监听的文件描述符
 * @param event     epoll事件
 * @param callback 回调函数
 * @param who       发送事件的对象是哪一类的
 * @param domain    AF_INET or AF_INET6
 * @return          事件配置结构体
 * @retval NULL     malloc失败
 */
EventConfig_t *initEvent(int epfd, int fd, uint32_t event, 
callback_t callback, uint8_t who, int domain);

/**
 * @brief 初始化循环
 * @param hash_num  多少条事件链表
 * @param cb        获得hash的回调函数
 * @return epoll_fd
 */
int loopInit(uint8_t hash_num, getHash_t cb);

/**
 * @brief 添加监听事件
 * @param event 事件结构体
 * @return      是否添加成功
 * @retval 0    成功
 * @retval -1   失败
 */
int eventAdd(EventConfig_t *event);

/**
 * @brief 删除监听事件
 * @param event 事件结构体
 * @return      是否删除成功
 * @retval 0    成功
 * @retval -1   失败
 */
int eventDel(EventConfig_t *event);

/**
 * @brief 修改监听事件
 * @param event 事件结构体
 * @return      是否删除成功
 * @retval 0    成功
 * @retval -1   失败
 */
int eventMod(EventConfig_t *event);

/**
 * @brief 开始循环
 * @param epfd  epoll监听描述符
 * @return      是否删除成功
 * @retval 0    成功
 * @retval -1   失败
 */
int loop(int epfd);

/**
 * @brief 结束循环
 * @param epfd  epoll监听描述符
 * @return      是否删除成功
 * @retval 0    成功
 * @retval -1   失败
 */
void loopDone(int epfd);

/**
 * @brief 提供接口给外部去遍历事件链表, 基于list封装
 * @param who           事件的类型
 * @param manipulate    操作函数(回调)
 * @param tag           提供给操作函数的额外参数
 */
void travelEventList(uint8_t who, manipulate_callback manipulate, void *tag);

/**
 * @brief 提供接口给外部去查找事件链表, 基于list封装
 * @param who       事件的类型
 * @param required  比较函数(回调)
 * @param tag       提供给操作函数的额外参数
 */
void *seachOneEventList(uint8_t who, required_callback required, void *tag);

/**
 * @brief 检查对应类型的eventlist是否为空链表
 * @param who       事件的类型
 * @return          是否成功
 * @retval true     成功
 * @retval false    失败
 */
bool isEmptyEventList(uint8_t who);

#endif //_EVENTLOOP_H