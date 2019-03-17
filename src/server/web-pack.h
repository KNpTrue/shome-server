/**
 * web-pack.h
 * 将传送给客户端的数据进行打包
 * 使用cJSON封装json传输给web端 
 * send:
 * {
 *    type: '',
 *    data: { }
 * }
 * recv:
 * {
 *    method: '',
 *    type: '',
 *    data: { }
 * }
*/
#include <stdint.h>

enum {
    PACK_DEV,
    PACK_TODO,
    PACK_SET,
    PACK_ROOM,
    PACK_WEBCONFIG,
    PACK_ALL,
};
/**
 * 打包数据
*/
char *json_packData(uint8_t type, void *tag, char *buf, int length);

uint32_t json_analysis(const char *src, char *dest, uint32_t destlen_max);