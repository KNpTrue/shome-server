//warp.c
#include "warp.h"

#include <errno.h>
#include <stdlib.h>
#include <unistd.h>

int Close(int fd)
{
    int ret;
again:
    ret = close(fd);
    if(ret == -1)
    {
        if(errno == EINTR)
            goto again;
        else
            return -1;
    }
    return ret;
}

int Accept(int s, struct sockaddr *addr, socklen_t *addrlen)
{
    int ret;
again:
    ret = accept(s, addr, addrlen);
    if(ret == -1)
    {
        if(errno == ECONNABORTED || errno == EINTR)
            goto again;
        else
            return -1;
    }
    return ret;
}

ssize_t Read(int fd, void *buf, size_t count)
{
    ssize_t ret;
again:
    ret = read(fd, buf, count);
    if(ret == -1)
    {
        if(errno == EINTR)
            goto again;
        else
            return -1;
    }
    return ret;
}

ssize_t Read_unblock(int fd, void *buf, size_t count)
{
    ssize_t rcount;
    //读数据
    do {
        rcount = Read(fd, buf, count);
        if(rcount == -1) //非组塞模式下可能为无数据可读
        {
            if(errno == EAGAIN) //无数据可读
                break;
            else //出现错误
                return -1;
        }
        else if(rcount == count) //可能没有读完, 由于本程序设计为小包发送, 未读完的直接丢弃
        {
            char tmp[count];
            //while(Read(connfd, tmp, BUF_LEN) < 0); //一种选择将剩余数据全部抛弃
            if(Read(fd, tmp, count) > 0) //超过buf
                return -1; //另外一种是直接将连接关闭
        }
        else if(rcount == 0) //客户端关闭
            return -1;
    } while (0);
    return rcount;
}

ssize_t Write(int fd, const void *buf, size_t count)
{
    ssize_t ret;
again:
    ret = write(fd, buf, count);
    if(ret == -1)
    {
        if(errno == EINTR)
            goto again;
#ifdef UNBLOCK //非阻塞
        else if(errno == EAGAIN)
        {
            usleep(100);
            goto again;
        }
#endif //UNBLOCK
        else
            return -1;
    }
    return ret;
}
