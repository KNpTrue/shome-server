/**
 * web-pack.h
 * 将传送给客户端的数据进行打包
 * 使用cJSON封装json传输给web端 
 * send:
 * {
 *    type: '', // =类型
 *    data: { } // 数据
 * }
 * 将接受的json数据解析出来并同步的服务器中
 * recv:
 * {
 *    method: '', // set or get
 *    type: '', // 类型
 *    data: { } // 数据
 * }
*/
#include <stdint.h>

enum pack_type {
    PACK_DEV,  //DevConfig_t
    PACK_TODO, //todo_t
    PACK_SET,  //task_set_t
    PACK_ROOM, //room_t
    PACK_WEBCONFIG, //WebConfig_t
    PACK_ALL, // NULL
};
/**
 * 打包数据
 * @type: 打包数据的类型
 * @tag: 根据打包类型传入要打包的结构体
*/
char *json_packData(uint8_t type, void *tag, char *buf, int length);
/**
 * 解析json数据包
*/
uint32_t json_analysis(const char *src, char *dest, uint32_t destlen_max);