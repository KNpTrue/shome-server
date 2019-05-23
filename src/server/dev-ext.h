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

// ext id 
typedef unsigned int id_t;

enum _condition_type {
    CON_SENSOR,
    CON_TIME
};
// room/set/todo 都继承于该结构体，它描述了基本信息
typedef struct ext_base {
    id_t      id;
    char      name[NAME_LEN];
} ext_base_t;
/**
 * 获取ext_base_t 链表头指针
*/

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
    ext_base_t base;
    char      devid[ID_LEN + 1];
    _key_t    key;
    void      *devConfig;
    bool      isUpdated;
} task_dev_t;
//task_set
typedef struct task_set {
    ext_base_t  base;
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
    ext_base_t  base;
    condition_t condition;  //触发条件
    task_set_t  *set;       //任务集
    id_t      set_id;     //任务集id
} todo_t;
//room
typedef struct room_dev {
    char id[ID_LEN + 1];
    void *devConfig;
} room_dev_t;

typedef struct room {
    ext_base_t  base;
    node_t      *roomDevList_head;  //设备链表
} room_t;

#endif //_DEV_dETAIL_H