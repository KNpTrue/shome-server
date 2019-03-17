//video.c
#include "video.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <linux/videodev2.h>

typedef struct {
    void *addr;
    uint32_t len;
} memInfo_t;

static memInfo_t memMap[V4L2_BUFS_COUNT];

int initVideo(const char *dev, uint32_t width, uint32_t height)
{
    int devfd = open(dev, O_RDWR); //打开设备文件
    if(devfd == -1)
    {
#ifdef DEBUG_DEV
        perror(dev);
#endif //DEBUG_DEV
        return -1;
    }
    //判断设备是不是视频设备
    struct v4l2_capability cap;
    if(ioctl(devfd, VIDIOC_QUERYCAP, &cap) == -1)
    {
#ifdef DEBUG_DEV
        perror("<initVideo>ioctl VIDIOC_QUERYCAP");
#endif //DEBUG_DEV
        return -1;
    }
    if(!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE))
    {
#ifdef DEBUG_DEV
        fprintf(stderr, "<initVideo>%s is not a video device.\n", dev);
#endif //DEBUG_DEV
        return -1;
    }
    //查看摄像头是否支持yuv422
    struct v4l2_fmtdesc fmtd;
    int index, isYUYV = 0;
    for(index = 0;; index++)
    {
        fmtd.index = index; //只有一个摄像头的设备
        fmtd.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        if(ioctl(devfd, VIDIOC_ENUM_FMT, &fmtd) == -1)
            break;
        if(fmtd.pixelformat == V4L2_PIX_FMT_YUYV)
        {
            isYUYV = 1;
            break;
        }
    }
    if(isYUYV == 0)
    {
#ifdef DEBUG_DEV
        fprintf(stderr, "<initVideo>%s does not support yuv422.\n", dev);
#endif //DEBUG_DEV
        return -1;
    }
    //设置采集格式
    struct v4l2_format fmt;
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width = width;
    fmt.fmt.pix.height = height;
    fmt.fmt.pix.pixelformat = fmtd.pixelformat; //YUYV
    if(ioctl(devfd, VIDIOC_S_FMT, &fmt) == -1)
    {
#ifdef DEBUG_DEV
        perror("<initVideo>ioctl VIDIOC_S_FMT");
#endif //DEBUG_DEV
        return -1;
    }
    //申请采集缓存
    struct v4l2_requestbuffers reqbufs;
    reqbufs.count = V4L2_BUFS_COUNT;
    reqbufs.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    reqbufs.memory = V4L2_MEMORY_MMAP;
    if(ioctl(devfd, VIDIOC_REQBUFS, &reqbufs) == -1)
    {
#ifdef DEBUG_DEV
        perror("<initVideo>ioctl VIDIOC_REQBUFS");
#endif //DEBUG_DEV
        return -1;
    }
    //mmap 将申请到的采集缓存映射到用户内存空间中
    struct v4l2_buffer buf;
    for(index = 0; index < V4L2_BUFS_COUNT; index++)
    {
        memset(&buf, 0, sizeof(buf));
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.index = index;
        if(ioctl(devfd, VIDIOC_QUERYBUF, &buf) == -1)
        {
#ifdef DEBUG_DEV
            perror("<initVideo>ioctl VIDIOC_QUERYBUF");
#endif //DEBUG_DEV
            return -1;
        }
        memMap[index].len = buf.length;
        memMap[index].addr = mmap(NULL, buf.length, PROT_READ, MAP_SHARED, devfd, buf.m.offset);
        if(memMap[index].addr == MAP_FAILED)
        {
            while(index--)
            {
                munmap(memMap[index].addr, memMap[index].len);
            }
#ifdef DEBUG_DEV
            perror("mmap");
#endif //DEBUG_DEV
            return -1;
        }
        //把buf放入采集队列中
        if(ioctl(devfd, VIDIOC_QBUF, &buf) == -1)
        {
            while(index--)
            {
                munmap(memMap[index].addr, memMap[index].len);
            }
#ifdef DEBUG_DEV
            perror("<initVideo>ioctl VIDIOC_QBUF");
#endif //DEBUG_DEV
            return -1;
        }
    }
    return devfd;
}

int startVideo(int devfd)
{
    enum v4l2_buf_type type;
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if(ioctl(devfd, VIDIOC_STREAMON, &type) == -1)
    {
#ifndef DEBUG_DEV
        perror("<startVideo>start failed");
#endif //DEBUG_DEV
        return -1;
    }
    return 0;
}

int stopVideo(int devfd)
{
    enum v4l2_buf_type type;
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if(ioctl(devfd, VIDIOC_STREAMOFF, &type) == -1)
    {
#ifndef DEBUG_DEV
        perror("<startVideo>stop failed");
#endif //DEBUG_DEV
        return -1;
    }
    return 0;
}

int closeVideo(int devfd)
{
    int index, failed = 0;
    if(stopVideo(devfd))
        return -1;
    for(index = 0; index < V4L2_BUFS_COUNT; index++)
    {
        if(munmap(memMap[index].addr, memMap[index].len))
        {
#ifdef DEBUG_DEV
            perror("<closeVideo>munmap");
#endif //DEBUG_DEV
            failed = 1;
        }
    }
    close(devfd);
    if(failed)
        return -1;
    else
        return 0;
}

int getYuvData(int devfd, uint8_t *data, uint32_t datalen_max)
{
    struct v4l2_buffer buf;
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    //出队采集好的数据，为采集好的阻塞
    if(ioctl(devfd, VIDIOC_DQBUF, &buf) == -1)
    {
#ifdef DEBUG_DEV
        perror("<getYuvData>ioctl VIDIOC_DQBUF");
#endif //DEBUG_DEV
        return -1;
    }
    int len = buf.bytesused;
    if(len > datalen_max)
    {
#ifdef DEBUG_DEV
        fprintf(stderr, "<getYuvData>datalen is too short.\n");
#endif //DEBUG_DEV
        ioctl(devfd, VIDIOC_QBUF, &buf);
        return -1;
    }
    memcpy(data, memMap[buf.index].addr, len);

    if(ioctl(devfd, VIDIOC_QBUF, &buf) == -1)
    {
#ifdef DEBUG_DEV
        perror("<getYuvData>ioctl VIDIOC_QBUF");
#endif //DEBUG_DEV
        return -1;
    }
    return len;
}