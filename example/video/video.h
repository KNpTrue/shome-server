//video.h
#ifndef _VIDEO_H
#define _VIDEO_H

#include <stdint.h>

#define    V4L2_BUFS_COUNT    4

/**
 * 初始化视频采集
 * @dev:  视频设备名
 * @return: 文件描述符
*/
int initVideo(const char *dev, uint32_t width, uint32_t height);
/**
 * 开始视频信号采集
 * @devfd: 设备文件描绘符
 * @return: 0 success -1 err
*/
int startVideo(int devfd);
/**
 * 停止视频信号采集
 * @devfd: 设备文件描绘符
 * @return: 0 success -1 err
*/
int stopVideo(int devfd);
/**
 * 关闭视频采集
 * @devfd: 设备文件描绘符
*/
int closeVideo(int devfd);
/**
 * 获得Yuv视频信号
 * @devfd: 设备文件描绘符
 * @data: 存放数据的地址
 * @datalen: 数据容量
 * @return: 成功返回data的大小， 错误返回-1
*/
long getYuvData(int devfd, uint8_t **data);

int returnYuvData(int devfd);

#endif //_VIDEO_H