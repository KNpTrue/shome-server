//dev-detail.h
#ifndef _DEV_DETAIL_H
#define _DEV_DETAIL_H

#include <stdbool.h>
#include <stdint.h>
#include "../list.h"
#include "../key.h"

#define   TIME_LEN       6
#define   ID_LEN         8
#define   NAME_LEN       32

enum _condition_type {
    CON_SENSOR,
    CON_TIME
};
//con_sensor
typedef struct con_sensor {
    char     id[ID_LEN + 1];  //设备id
    bool     flag; //保证只被触发一次
    _key_t   key;         //触发的key
    uint8_t  symbol;
    void     *devConfig;
} con_sensor_t;
//con_time
typedef struct con_time {
    char        time[TIME_LEN];
    uint8_t     days;
} con_time_t;
//task_dev
typedef struct task_dev {
    char      id[ID_LEN + 1];
    _key_t    key;
    void      *devConfig;
    bool      isUpdated;
} task_dev_t;
//task_set
typedef struct task_set {
    uint8_t      id;
    char        name[NAME_LEN];
    node_t      *task_devList_head;
    node_t      *regTodoList_head;
} task_set_t;
//condition todo->condition
typedef struct condition {
    uint8_t type; //条件类型
    union {
        con_sensor_t con_sensor;
        con_time_t   con_time;
    }       detail; //条件
} condition_t;
//toDo
typedef struct todo {
    uint8_t      id;
    char        name[NAME_LEN]; //事件名称
    condition_t condition;  //触发条件
    task_set_t  *set;       //任务集
    uint8_t      set_id;     //任务集id
} todo_t;
//room
typedef struct room_dev {
    char id[ID_LEN + 1];
    void *devConfig;
} room_dev_t;
typedef struct room {
    uint8_t     id;
    char        name[NAME_LEN]; //房间名
    node_t      *roomDevList_head;  //设备链表
} room_t;

#if 0
//智能开关
typedef struct sm_switch {

} sm_switch_t;
//传感器
typedef struct sm_sensor {

} sm_sensor_t;
//视频设备
typedef struct sm_video {

} sm_video_t;
#endif
#endif //_DEV_dETAIL_H