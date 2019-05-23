//event-socket.c
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <linux/tcp.h>


#include "event-socket.h"
#include "event-who.h"
#include "web-pack.h"
#include "../warp.h"
#include "../log.h"

//sockaddr
union sockaddr_types {
    struct sockaddr_in  in4;
    struct sockaddr_in6 in6;
};

//web
int _recvWebClient_cb(EventConfig_t *connEvent);
//dev
int _recvDevClient_cb(EventConfig_t *connEvent);

int socket_KeepAlive(int fd, int interval);

int initListenFd(int domain, short port)
{
    if(domain != AF_INET && domain != AF_INET6)
    {
        loge("<initListenFd>domain error.\n");
        return -1;
    }
    int listenfd = socket(domain, SOCK_STREAM, 0);
    if(listenfd == -1)
    {
        logp("<initListenFd>scoket err");
        return -1;
    }
    //设置端口复用
    int optval = 1;
	setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
    //bind
    union sockaddr_types serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    if(domain == AF_INET)  //ipv4
    {
        serv_addr.in4.sin_addr.s_addr = htonl(INADDR_ANY);
        serv_addr.in4.sin_port = htons(port);
        serv_addr.in4.sin_family = AF_INET;
    }
    else  //ipv6
    {
        serv_addr.in6.sin6_addr = in6addr_any;
        serv_addr.in6.sin6_port = htons(port);
        serv_addr.in6.sin6_family = AF_INET6;
    }
    if(bind(listenfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)))
    {
        logp("<initListenFd>bind err");
        return -1;
    }
    logd("server start! port: %d\n", port);
    listen(listenfd, MAX_LISTEN);
    return listenfd;
}

int waitClient_cb(EventConfig_t *listenEvent)
{
    int connfd;
    if(listenEvent->domain == AF_INET) //ipv4
    {
        struct sockaddr_in client_addr;
        memset(&client_addr, 0, sizeof(client_addr));
        socklen_t client_addr_len = sizeof(client_addr);
        connfd = Accept(listenEvent->fd, (struct sockaddr *)&client_addr, &client_addr_len);
        if(connfd == -1)
        {
            logp("<waitWebClient_cb>accept err");
            return -1;
        }
        else //success
        {
            char client_ip[INET_ADDRSTRLEN];
            logd("<server>client addr: %s port: %d\n", inet_ntop(AF_INET, 
            &client_addr.sin_addr.s_addr, client_ip, INET_ADDRSTRLEN), ntohs(client_addr.sin_port));
        }
    }
    else //ipv6
    {
        struct sockaddr_in6 client_addr;
        memset(&client_addr, 0, sizeof(client_addr));
        socklen_t client_addr_len = sizeof(client_addr);
        connfd = Accept(listenEvent->fd, (struct sockaddr *)&client_addr, &client_addr_len);
        if(connfd == -1)
        {
            logp("<waitWebClient_cb>accept err");
            return -1;
        }
        else //success
        {
            char client_ip[INET6_ADDRSTRLEN];
            logd("<server>client addr: %s port: %d\n", inet_ntop(AF_INET6,
            &client_addr.sin6_addr, client_ip, INET6_ADDRSTRLEN), ntohs(client_addr.sin6_port));
        }
    }
    //设置成非阻塞
    int flag = fcntl(connfd, F_GETFL);
    fcntl(connfd, F_SETFL, flag | O_NONBLOCK);
    socket_KeepAlive(connfd, 9);
    EventConfig_t *connEvent;
    //判断是谁的事件并初始化事件
    switch(listenEvent->who)
    {
    case WEBSOCKET_LISTENER: //websocket
        connEvent = eventInit(listenEvent->epfd, connfd, EPOLLIN, recvClient_cb, sendClient_cb,
                            WEBSOCKET_CONNECTOR | WEBSOCKET_FIRSTCONN, listenEvent->domain);
        connEvent->tag = listenEvent->tag;
        break;
    case DEVICE_LISTENER: //device
        connEvent = eventInit(listenEvent->epfd, connfd, EPOLLIN, recvClient_cb, sendClient_cb, 
                            DEVICE_CONNECTOR | DEVICE_FIRSTCONN, listenEvent->domain);
    }
    if(connEvent == NULL) //如果失败
    {
        loge("<waitWebClient_cb>initAccept err, close connfd.\n");
        close(connfd);
        return 0;
    }
    //将描述符挂到树上 ET模式
    eventAdd(connEvent);
    return 0;
}

int recvClient_cb(EventConfig_t *connEvent)
{
    if(connEvent->who & WEBSOCKET_CONNECTOR) 
        return _recvWebClient_cb(connEvent);
    else if(connEvent->who & DEVICE_CONNECTOR)
        return _recvDevClient_cb(connEvent);
    else
        return 0;
}

int sendClient_cb(EventConfig_t *connEvent)
{
    char *buf = connEvent->buf;
    logd("%s\nbuflen: %d\nsend: ", buf, connEvent->buflen);
    Write(connEvent->fd, buf, connEvent->buflen); //回写客户端
    //将事件切换成读事件
    _switchEventMode(connEvent, EPOLLIN);
    return 0;
}

int _recvWebClient_cb(EventConfig_t *connEvent)
{
    bool isSwitchWrite = true;  //默认情况下切换读写事件
    int connfd = connEvent->fd;
    char *buf = connEvent->buf;
    int rcount;
    //读数据
    rcount = Read_unblock(connfd, buf, BUF_LEN);
    if(rcount == -1)    goto err;
    buf[rcount] = '\0';  //保证数据正确
#ifdef DEBUG
    printf("%s\n", buf);
    int i;
    for(i = 0; i < connEvent->buflen; i++)
    {
        printf("%02X", (unsigned char)connEvent->buf[i]);
    }
    printf("\n");
#endif //DEBUG
    if(connEvent->who & WEBSOCKET_FIRSTCONN) //第一次连接客户端, 需要先握手
    {
        connEvent->who &= ~WEBSOCKET_FIRSTCONN; //将第一次握手标志去掉
        //key
        char srckey[WEBSOCKET_KEY_LEN];
        const char *key_flag[] = {"Sec-WebSocket-Key: ", "Sec-WebSocket-Protocol: "};
        //从HTTP头中找到websocket-key
        if(!getWebSocketValue(buf, srckey, WEBSOCKET_KEY_LEN, key_flag[0])) 
        {
            loge("<recvWebClient_cb>getWebSocketValue: Can't get websocket-key, close connect.\n");
            goto err;
        }
        char subkey[WEBSOCKET_KEY_LEN];
        if(!getWebSocketValue(buf, subkey, WEBSOCKET_KEY_LEN, key_flag[1]))
        {
            loge("<recvWebClient_cb>getWebSocketValue: Can't get websocket-protocol, close connect.\n");
            goto err;
        }
        //认证
        if(!isSubKeyRight(subkey)) //认证错误
        {
            loge("<recvWebClient_cb>isSubKeyRight: sub-Key error, close connect.\n");
            goto err;
        }
        //编码websocket-key
        char destkey[WEBSOCKET_KEY_LEN];
        if(!encodeWebSocketKey(srckey, destkey, WEBSOCKET_KEY_LEN))
            goto err;
        //将消息头存到buf中
        repondHeadBuild(buf, destkey, subkey);
        connEvent->buflen = strlen(buf);
    }
    else //不是第一次握手
    {
        //解包
        char depackage_data[BUF_LEN];
        unsigned int depackage_data_len;
        int ret = dePackage((uint8_t *)buf, rcount, (uint8_t *)depackage_data, 
                            BUF_LEN, &depackage_data_len);
        if(ret == OP_ERR || ret == OP_DISCONN || ret == OP_CONTINUE)
        {
            logd("<websocket> opCode: %02X\n", ret);
            goto err;
        }
        else  //处理包数据, 包括ping和pong，都由用户处理
        {
            if(connEvent->tag) //附加参数使用
            {
                char dealed_data[BUF_LEN];
                webDealData_cb dealData = (webDealData_cb)connEvent->tag; //回调函数
                unsigned int dealed_data_len = dealData(&ret, depackage_data, 
                depackage_data_len, dealed_data, BUF_LEN);
                if(dealed_data_len == 0)    isSwitchWrite = false; //如果返回0
                //处理完后再打包放入buf(表达式短路)
                isSwitchWrite && (connEvent->buflen = enPackage(ret, 0, 
                (uint8_t *)dealed_data, dealed_data_len, (uint8_t *)buf, BUF_LEN));
            }
            else //直接将数据原样返回
            {
                //处理完后再打包放入buf(表达式短路)
                isSwitchWrite && (connEvent->buflen = enPackage(ret, 0, 
                (uint8_t *)depackage_data, depackage_data_len, (uint8_t *)buf, BUF_LEN));
            }
        }
    }
    //将事件切换成写事件
    isSwitchWrite && _switchEventMode(connEvent, EPOLLOUT);
    return 0;
err:
    //关闭描述符，并从树上摘下
    if(eventDel(connEvent))   return -1;
    return 0;
}

int _recvDevClient_cb(EventConfig_t *connEvent)
{
    bool isSwitchWrite = true;  //默认情况下切换读写事件
    int connfd = connEvent->fd;
    char *buf = connEvent->buf;
    int rcount;
    DevConfig_t *devConfig;
    //读数据
    rcount = Read_unblock(connfd, buf, BUF_LEN);
    if(rcount == -1)    goto err;
    buf[rcount] = '\0';  //保证数据正确
    char buf2[BUF_LEN];
    uint32_t len;
    len = dev_dePackage(buf, rcount, buf2, BUF_LEN);
#ifdef DEBUG
    logd("<_recvDevClient_cb>buflen: %d\n", len);
    for(int i = 0; i < len; i++)
    {
        logd("%02X ", (unsigned char)buf2[i]);
    }
    logd("\n");
#endif //DEBUG
    if(connEvent->who & DEVICE_FIRSTCONN) //first
    {
        //检测设备
        if(*buf2 == DEV_PRO_UPDATE) //掉线重连时设备端不知道，需要告诉设备端重置
        {
            char tmp = DEV_PRO_RST;
            connEvent->buflen = dev_enPackage(&tmp, sizeof(tmp), buf, BUF_LEN, rand);
            goto done;
        }
        connEvent->who &= ~DEVICE_FIRSTCONN; //将第一次的标志位去除
        //握手
        devConfig = dev_handShake(buf2, len);
        if(devConfig == NULL)   goto err;
        devConfig->ep_event = connEvent;
        connEvent->tag = (void *)devConfig;
        strcpy(buf2, DEV_MAGIC);
        connEvent->buflen = dev_enPackage(buf2, strlen(DEV_MAGIC), buf, BUF_LEN, rand);
    }
    else //处理数据
    {
        devConfig = (DevConfig_t *)connEvent->tag;
        if(dev_getData(buf2, len, connEvent->tag))
        {
            if(buf2[0] == DEV_MAGIC[0] && buf2[1] == DEV_MAGIC[1] && buf2[2] == DEV_MAGIC[2])
            {
                strcpy(buf2, DEV_MAGIC);
                len = dev_enPackage(buf2, strlen(DEV_MAGIC), buf, BUF_LEN, rand);
                connEvent->buflen = len;
                goto done;
            }
            loge("error.\n");
            goto err;
        }
        isSwitchWrite = false;
        //将数据传送给web端
        sendDataToWeb(PACK_DEV, devConfig);
        node_t *keylist_head = devConfig->keyList_head;
        if(devConfig->todolist_head) //触发事件
        {                
            node_t *todolist_head = seachByRequired(devConfig->todolist_head,
                    (required_callback)isMeetCon_sensor, devConfig);
            if(todolist_head)
            {
                travelList(todolist_head, (manipulate_callback)runTodo, NULL);
            }
            deleteList(&todolist_head, NULL);
        }
        if(devConfig->tasklist_head) //更新列表
            travelList(devConfig->tasklist_head, 
                    (manipulate_callback)updateTaskDev, keylist_head);
        //打印
#ifdef DEBUG
        travelList(keylist_head, (manipulate_callback)printKey, NULL);
#endif //DEBUG
    }
done:
    //将事件切换成写事件
    isSwitchWrite && _switchEventMode(connEvent, EPOLLOUT);
    return 0;
err:
    //关闭描述符，并从树上摘下
    if(connEvent->tag)
    {
        devConfig = (DevConfig_t *)connEvent->tag;
        devConfig->isOnline = false;
        devConfig->ep_event = NULL;
        deleteList(&devConfig->keyList_head, free);

        sendDataToWeb(PACK_DEV, devConfig);
    }
    if(eventDel(connEvent))   return -1;
    return 0;
}
//发送json数据给单个web端
void _sendDataToWeb(EventConfig_t *evt, const char *data)
{
    evt->buflen = enPackage(OP_TXTDATA, false, (uint8_t *)data, 
            strlen(data), (uint8_t *)evt->buf, BUF_LEN);
    _switchEventMode(evt, EPOLLOUT);
}

void sendDataToWeb(uint8_t type, void *tag)
{
    if(!isEmptyEventList(WEBSOCKET_CONNECTOR))
    {
        char *data = malloc(BUF_LEN);
        travelEventList(WEBSOCKET_CONNECTOR, (manipulate_callback)_sendDataToWeb, 
                json_packData(type, tag, data, BUF_LEN));
        free(data);
    }
}

int socket_KeepAlive(int fd, int interval)
{
    int val = 1;
	//开启keepalive机制
    if (setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &val, sizeof(val)) == -1)
    {
        goto err;
    }

    val = interval;
    if (setsockopt(fd, IPPROTO_TCP, TCP_KEEPIDLE, &val, sizeof(val)) < 0) {
        goto err;
    }

    val = interval/3;
    if (val == 0) val = 1;
    if (setsockopt(fd, IPPROTO_TCP, TCP_KEEPINTVL, &val, sizeof(val)) < 0) {
        goto err;
    }
 
    val = 3;
    if (setsockopt(fd, IPPROTO_TCP, TCP_KEEPCNT, &val, sizeof(val)) < 0) {
        goto err;
    }
 
    return 0;
err:
    logp("setsockopt SO_KEEPALIVE: %s");
    return -1;
}
