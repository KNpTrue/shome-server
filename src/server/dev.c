//dev.c
#include "dev.h"
#include "dev-time.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "event-socket.h"

//todo-list
node_t *todolist_head = NULL;
//all devices include conf and connect
node_t *devlist_head = NULL;
//set-list
node_t *setlist_head = NULL;
//room-list
node_t *roomlist_head = NULL;
//
void setNull_cb(void *data, uint8_t *type);

/**
 * 每次都返回最大的ID
*/
void getMaxId(ext_base_t *ext, int *max)
{
    if (!ext)   return;
    if ((int)ext->id > *max)   *max = ext->id;
}

int ext_getNewId(node_t *head)
{
    int max = -1;
    travelList(head, (manipulate_callback)getMaxId, &max);
    
    return max + 1;
}

void ext_sortId_cb(ext_base_t *ext, int *tmp)
{
    ext->id = *tmp + 1;
    (*tmp)++;
}


bool ext_isId(ext_base_t *ext, const id_t *id)
{
    return ext->id == *id;
}

//dev
DevConfig_t *initDevConfig()
{
    DevConfig_t *dev = malloc(sizeof(DevConfig_t));
    if(!dev)    return NULL;
    memset(dev->id, 0, ID_LEN + 1);
    dev->isOnline = false;
    dev->keyList_head = NULL;
    dev->todolist_head = NULL;
    dev->tasklist_head = NULL;
    dev->ep_event = NULL;
    dev->room = NULL;
    return dev;
}

bool isDevId(DevConfig_t *devConfig, const char *id)
{
    return strncmp(devConfig->id, id, ID_LEN) == 0;
}

void destoryDevConfig(DevConfig_t *devConfig)
{
    if(!devConfig)  return;
    if(devConfig->keyList_head)
        deleteList(&devConfig->keyList_head, free);
    uint8_t set_type;
    if(devConfig->todolist_head)
    {
        set_type = KEY_READONLY;
        travelList(devConfig->todolist_head, (manipulate_callback)setNull_cb, &set_type);
        deleteList((node_t **)&devConfig->todolist_head, NULL);
    }
    if(devConfig->tasklist_head)
    {
        set_type = KEY_READWRITE;
        travelList(devConfig->tasklist_head, (manipulate_callback)setNull_cb, &set_type);
        deleteList((node_t **)&devConfig->tasklist_head, NULL);
    }
    if(devConfig->ep_event) ((EventConfig_t *)devConfig->ep_event)->tag = NULL;
    if(devConfig->room && devConfig->room->roomDevList_head)
        deleteNode(&devConfig->room->roomDevList_head, (required_callback)isRoomDevId, 
                    devConfig->id, (destory_callback)destoryRoomDev);
    free(devConfig);
}

void setNull_cb(void *data, uint8_t *type)
{
    switch(*type)
    {
    case KEY_READONLY:;
        todo_t *todo = data;
        todo->condition.detail.con_sensor.devConfig = NULL;
        break;
    case KEY_READWRITE:;
        task_dev_t *task = data;
        task->devConfig = NULL;
        break;
    }
}

//todo
void registerTodo(todo_t *todo)
{
    if(todo->set)   return;
    task_set_t *set = seachOneByRequired(setlist_head, 
            (required_callback)ext_isId, &todo->set_id);
    todo->set = set;
    if(set) appendList(&set->regTodoList_head, todo);
    if(todo->condition.type == CON_SENSOR)
    {
        con_sensor_t *con_sensor = &todo->condition.detail.con_sensor;
        if(con_sensor->devConfig)  return;
        DevConfig_t *dev = seachOneByRequired(devlist_head, 
                    (required_callback)isDevId, con_sensor->id);
        con_sensor->devConfig = dev;
        if(dev) appendList((node_t **)&dev->todolist_head, todo);
    }
    else    updateNextAlarm(todo);
}

void destoryTodo(todo_t *todo)
{
    if(!todo)   return;
    if(todo->set && todo->set->regTodoList_head)
        deleteNode(&todo->set->regTodoList_head, 
                (required_callback)ext_isId, &todo->base.id, NULL);
    if(todo->condition.type == CON_SENSOR)
    {
        DevConfig_t *dev = (DevConfig_t *)todo->condition.detail.con_sensor.devConfig;
        if(dev && dev->todolist_head)
            deleteNode((node_t **)&dev->todolist_head, (required_callback)ext_isId, &todo->base.id, NULL);
    }
    free(todo);
}

bool isMeetCon_sensor(todo_t *todo, DevConfig_t *devConfig)
{
    if(todo->condition.type != CON_SENSOR)
        return false;
    con_sensor_t *con_sensor = &todo->condition.detail.con_sensor;
    _key_t *key = seachOneByRequired(devConfig->keyList_head, 
        (required_callback)isSameKeyName, con_sensor->key.name);
    if(isKeyRequired(key, &con_sensor->key, con_sensor->symbol)) //true
    {
        if(con_sensor->flag == true)    return false;
        con_sensor->flag = true;
        return true;
    }
    else
    {
        con_sensor->flag = false;
        return false;
    }
}

void runTodo(todo_t *todo)
{
    if(todo && todo->set)   runTaskSet(todo->set);
}

//task-set
task_set_t *initTaskSet()
{
    task_set_t *set = malloc(sizeof(task_set_t));
    if(!set)    return NULL;
    set->task_devList_head = NULL;
    set->regTodoList_head = NULL;
    return set;
}       

void destoryTaskSet(task_set_t *set)
{
    if(!set)    return;
    if(set->task_devList_head)
        deleteList(&set->task_devList_head, (destory_callback)destoryTaskDev);
    if(set->regTodoList_head)
        deleteList(&set->regTodoList_head, NULL);
    free(set);
}

void registerSet(task_set_t *set)
{
    travelList(set->task_devList_head, (manipulate_callback)registerTaskDev, NULL);
}

bool isSetId(task_set_t *set, const uint8_t *id)
{
    return set->base.id == *id;
}

void runTaskSet(task_set_t *set)
{
    if(set && set->task_devList_head)
        travelList(set->task_devList_head, (manipulate_callback)runTaskDev, NULL);
}

//task-dev
task_dev_t *initTaskDev()
{
    task_dev_t *task = malloc(sizeof(task_dev_t));
    if(!task)    return NULL;
    task->devConfig = NULL;
    task->isUpdated = false;
    return task;
}

void registerTaskDev(task_dev_t *task)
{
    if(task->devConfig) return;
    DevConfig_t *dev = seachOneByRequired(devlist_head, 
                    (required_callback)isDevId, task->devid);
    task->devConfig = dev;
    if(dev) appendList((node_t **)&dev->tasklist_head, task);
}

void destoryTaskDev(task_dev_t *task)
{
    if(!task)   return;
    DevConfig_t *dev = (DevConfig_t *)task->devConfig;
    if(dev)
        deleteNode((node_t **)&dev->tasklist_head, (required_callback)ext_isId, &task->base.id, NULL);
    free(task);
}

void runTaskDev(task_dev_t *task)
{
    if(task && task->isUpdated) return;
    EventConfig_t *evt = ((DevConfig_t *)task->devConfig)->ep_event;
    if(!evt)    return;
    memset(evt->buf, 0, BUF_LEN);
    char buf[BUF_LEN];
    uint32_t len = dev_makeData(buf, &task->key);
    evt->buflen = dev_enPackage(buf, len, evt->buf, BUF_LEN, rand);
    _switchEventMode(evt, EPOLLOUT);
}

void updateTaskDev(task_dev_t *task, node_t *keylist_head)
{
    _key_t *key = seachOneByRequired(keylist_head, 
        (required_callback)isSameKeyName, task->key.name);
    if(!key)    return;
    if(!memcmp(&task->key.value, &key->value, getValueSize(key->type)))
        task->isUpdated = true;
    else task->isUpdated = false;
}
//room
room_t *initRoom()
{
    room_t *room = malloc(sizeof(room_t));
    if(room == NULL)    return NULL;
    room->roomDevList_head = NULL;
    return room;
}

void registerRoom(room_t *room)
{
    if(!room->roomDevList_head) return;
    travelList(room->roomDevList_head, (manipulate_callback)registerRoomDev, room);
}
void destoryRoom(room_t *room)
{
    if(room->roomDevList_head)
        deleteList(&room->roomDevList_head, (destory_callback)destoryRoomDev);
    free(room);
}

void sortRoomId(room_t *room, uint8_t *tmp)
{
    room->base.id = *tmp + 1;
    (*tmp)++;
}

//room_dev
room_dev_t *initRoomDev()
{
    room_dev_t *room_dev = malloc(sizeof(room_dev_t));
    if(!room_dev)    return NULL;
    memset(room_dev, 0, sizeof(room_dev_t));
    return room_dev;
}
void destoryRoomDev(room_dev_t *room_dev)
{
    if(room_dev->devConfig)
        ((DevConfig_t *)room_dev->devConfig)->room = NULL;
    free(room_dev);
}

void registerRoomDev(room_dev_t *room_dev, room_t *room)
{
    if(!room_dev)    return;
    room_dev->devConfig = seachOneByRequired(devlist_head, 
                (required_callback)isDevId, room_dev->id);
    if(room_dev->devConfig) ((DevConfig_t *)room_dev->devConfig)->room = room;
}

bool isRoomDevId(room_dev_t *room_dev, const char *id)
{
    if(!strncmp(room_dev->id, id, ID_LEN))   return true;
    else  return false;
}

//days
int setDays(con_time_t *con_time, const char *buf) // '*'表示该位已设置
{
    if(strlen(buf) < 8) return -1;
    uint8_t *days = &con_time->days;
    uint8_t i;
    *days = 0;
    for(i = 0; i < 7; i++)
    {
        if(buf[i] == '*')  *days |= (1 << i);
    }
    return 0;
}

char *daysToStr(con_time_t *con_time, char *buf, uint8_t buflen)
{
    if(buflen < 8) return NULL;
    int i;
    for(i = 0; i <= 6; i++)
    {
        if(con_time->days & (1 << i))   buf[i] = '*';
        else buf[i] = ' ';
    }
    buf[7] = ' ';
    if(buflen > 8)  buf[8] = '\0';
    return buf;
}

#ifdef DEBUG_DEV
void printDev(DevConfig_t *dev)
{
    printf("<dev>\tid:%s \tname:%s \ttype:%d \t", dev->id, dev->name, dev->type);
    travelList(dev->keyList_head, (manipulate_callback)printKey, NULL);
    putchar('\n');
}

void printTodo(todo_t *todo)
{
    printf("<todo>\tid:%d \tname:%s \tset_id:%d \t", todo->base.id, todo->base.name, todo->set_id);
    switch(todo->condition.type)
    {
    case CON_SENSOR:
        printf("sensor id:%s \tsymbol:%d \t", todo->condition.detail.con_sensor.id,
                todo->condition.detail.con_sensor.symbol);
        printKey(&todo->condition.detail.con_sensor.key);
        break;
    case CON_TIME:
        printf("time:%s \t", todo->condition.detail.con_time.time);
        printf("days:%02x\n", todo->condition.detail.con_time.days);
        break;
    }
}

void printSet(task_set_t *set)
{
    printf("<set>\tid:%d \tname:%s", set->base.id, set->base.name);
    travelList(set->task_devList_head, (manipulate_callback)printTask, NULL);
}

void printTask(task_dev_t *task)
{
    printf("\t<task>id:%d devid:%s\t", task->base.id, task->devid);
    printKey(&task->key);
}

void printRoom(room_t *room)
{
    printf("<room>\tid:%d \tname:%s", room->base.id, room->base.name);
    travelList(room->roomDevList_head, (manipulate_callback)printRoomDev, NULL);
    putchar('\n');
}

void printRoomDev(room_dev_t *room_dev)
{
    printf("\t<room_dev>id:%s \t", room_dev->id);
}

#endif //DEBUG_DEV