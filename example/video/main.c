//main.c
#include "video.h"

#include <stdio.h>
#include <signal.h>
#include <sys/mman.h>
#include <linux/fb.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>

#define  DEV_PATH  "/dev/video0"
#define  FB_PATH   "/dev/fb0"
#define  VIDEO_W   1280
#define  VIDEO_H   720

#define clamp(v) (v < 0 ? 0 : (v > 255 ? 255 : v))

void initSignal();
//SIGINT处理函数 
void sigint_cb(int signo);
//初始化fb
int initFb(const char *fbpath, uint8_t **map, uint32_t *maplen, uint32_t *fb_w, uint32_t *fb_h);
int closeFb(int fbfd, uint8_t *map, uint32_t maplen);

static int devfd;
static int fbfd;
static uint32_t maplen;
static uint8_t *map;
static uint8_t *yuyv;

int main()
{
    initSignal();
    devfd = initVideo(DEV_PATH, VIDEO_W, VIDEO_H);
    if(devfd == -1) return -1;
    //fb
    uint32_t fb_w, fb_h;
    fbfd = initFb(FB_PATH, &map, &maplen, &fb_w, &fb_h);
    if(fbfd == -1)  return -1;
    uint32_t yuyvlen;
    uint32_t yuyvlen_max = VIDEO_W * VIDEO_H << 1;
    uint8_t *p;
    uint32_t i, j;
    int y0, u0, y1, v1;
    yuyv = malloc(yuyvlen_max);
    //开始采集
    startVideo(devfd);
    while(1)
    {
        yuyvlen = getYuvData(devfd, yuyv, yuyvlen_max);
        if(yuyvlen == -1)   break;
        p = yuyv;
        for(i = 0; i < VIDEO_H; i++)
        {
            for(j = 0; j < VIDEO_W; j+=2)
            {
                y0 = p[0], u0 = p[1], y1 = p[2], v1 = p[3];
                p += 4;
                
                map[(i * fb_w + j) * 4 + 2] = clamp(y0 + 1.402 * (v1 -128)); //r
				map[(i * fb_w + j) * 4 + 1] = clamp(y0 - 0.344 * (u0 - 128) - 0.714 * (v1 -128)); // g
				map[(i * fb_w + j) * 4 + 0] = clamp(y0 + 1.772 * (u0 - 128)); //b
	
				map[(i * fb_w + j + 1) * 4 + 2] = clamp(y1 + 1.402 * (v1 -128)); //r
				map[(i * fb_w + j + 1) * 4 + 1] = clamp(y1 - 0.344 * (u0 - 128) - 0.714 * (v1 -128)); // g
				map[(i * fb_w + j + 1) * 4 + 0] = clamp(y1 + 1.772 * (u0 - 128)); //b
            }
        }
    }
    free(yuyv);
    closeFb(fbfd, map, maplen);
    closeVideo(devfd);
    return 0;
}

void initSignal()
{
    struct sigaction act;
    act.sa_flags = 0;
    act.sa_handler = sigint_cb;
    sigaction(SIGINT, &act, NULL);
}

void sigint_cb(int signo)
{
    free(yuyv);
    closeFb(fbfd, map, maplen);
    closeVideo(devfd);
    exit(0);
}

int initFb(const char *fbpath, uint8_t **map, uint32_t *maplen, uint32_t *fb_w, uint32_t *fb_h)
{
    int fb = open(fbpath, O_RDWR);
    if(fb == -1)
    {
#ifdef DEBUG_DEV
        perror(fbpath);
#endif //DEBUG_DEV
        return -1;
    }
    struct fb_fix_screeninfo finfo;
    if(ioctl(fb, FBIOGET_FSCREENINFO, &finfo) == -1)
    {
#ifdef DEBUG_DEV
        perror("<initFb>ioctl FBIOGET_FSCREENINFO");
#endif //DEBUG_DEV
        return -1;
    }
    struct fb_var_screeninfo vinfo;
    if(ioctl(fb, FBIOGET_VSCREENINFO, &vinfo) == -1)
    {
#ifdef DEBUG_DEV
        perror("<initFb>ioctl FBIOGET_VSCREENINFO");
#endif //DEBUG_DEV
        return -1;
    }
    *fb_w = finfo.line_length / (vinfo.bits_per_pixel >> 3);
    *fb_h = vinfo.yres;
    *maplen = finfo.line_length * (*fb_h);
    *map = mmap(NULL, *maplen, PROT_READ | PROT_WRITE, MAP_SHARED, fb, 0);
    if(*map == MAP_FAILED)
    {
#ifdef DEBUG_DEV
        perror("<initFb>mmap");
#endif //DEBUG_DEV
        return -1;
    }
    return fb;
}

int closeFb(int fbfd, uint8_t *map, uint32_t maplen)
{
    munmap(map, maplen);
    close(fbfd);
}