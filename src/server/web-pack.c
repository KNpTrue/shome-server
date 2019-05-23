/**
 * web.c
 * 为了能够保证服务端的稳定性（绝对不准许服务端突然崩溃），
 * 必须在用数据之前先检查数据是否可用，
 * 避免使用到不存在的或者不安全的数据
 */
#include "web-pack.h"
#include <cJSON.h> //包含json相关操作函数的库
#include "dev.h"
#include "web-config.h"
#include "event-socket.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

enum {
    METHOD_GET,
    METHOD_SET,
};

enum {
    TYPE_ALL,
    TYPE_DEV,
    TYPE_ROOM,
    TYPE_TODO,
    TYPE_SET
};

char *pack_typeToString(uint8_t type);
void pack_addDev(DevConfig_t *devConfig, cJSON *data); //void * -> DevConfig_t *
void pack_addTodo(todo_t *todo, cJSON *data); //void * -> todo_t *
void pack_addSet(task_set_t *set, cJSON *data); //void * -> todo_t *
void pack_addRoom(room_t *room, cJSON *data); //void * -> room_t *
void pack_addWebConfig(WebConfig_t *webConfig, cJSON *data); //void * -> room_t *
void pack_addAll(cJSON *data);
void addKeyToArray(_key_t *key, cJSON *array); //call_back
void addKeyToObj(_key_t *key, cJSON *obj, bool packModeAndUnit);
void addTaskToArray(task_dev_t *task, cJSON *array); //call_back
void addRoomDevIdToArray(room_dev_t *room_dev, cJSON *array); //call_back

void addDevToArray(DevConfig_t *dev, cJSON *array);
void addTodoToArray(todo_t *todo, cJSON *array);
void addSetToArray(task_set_t *set, cJSON *array);
void addRoomToArray(room_t *room, cJSON *array);
//从JSON数据中读出key
int getKeyByJson(_key_t *key, cJSON *what);

char *json_packData(uint8_t type, void *tag, char *buf, int length)
{
    cJSON *root = cJSON_CreateObject();
    if(root == NULL)    
    {
#ifdef DEBUG_WEB
        fprintf(stderr, "<packData>cJSON create error.\n");
#endif //DEBUG_WEB
        return NULL;
    }
    cJSON_AddStringToObject(root, "type", pack_typeToString(type)); // "type": ""
    cJSON *data = cJSON_AddObjectToObject(root, "data"); // "data": {}
    //根据type往data中添加数据
    switch(type)
    {
    case PACK_DEV: pack_addDev(tag, data); break; 
    case PACK_TODO: pack_addTodo(tag, data); break;
    case PACK_SET: pack_addSet(tag, data); break;
    case PACK_ROOM: pack_addRoom(tag, data); break;
    case PACK_WEBCONFIG: pack_addWebConfig(tag, data); break;
    case PACK_ALL: pack_addAll(data); break;
    }
    //将json数据转换为字符串
    if(!cJSON_PrintPreallocated(root, buf, length, false))  goto err;
    cJSON_Delete(root);
    return buf;
err:
    cJSON_Delete(root);
    return NULL;
}

uint32_t json_analysis(const char *src, char *dest, uint32_t destlen_max)
{
    if(!src || !dest || destlen_max == 0)   goto err;
    uint32_t ret = 0;
    cJSON *root = cJSON_Parse(src);  if(!root)   goto err;
    cJSON *method = cJSON_GetObjectItem(root, "method");    if(!method) goto err;
    cJSON *type = cJSON_GetObjectItem(root, "type");    if(!type)   goto err; 
    cJSON *data = cJSON_GetObjectItem(root, "data");    if(!data)   goto err;
    if(method->valueint == METHOD_GET) //GET
    {
        switch(type->valueint) //TYPE
        {
        case TYPE_ALL:
            if(json_packData(PACK_ALL, NULL, dest, destlen_max))
                ret = strlen(dest);
            else goto err;
            break;
        default: break;
        }
    }
    else if(method->valueint == METHOD_SET) //SET
    {
        cJSON *id = cJSON_GetObjectItem(data, "id");    if(!id) goto err;
        cJSON *who = cJSON_GetObjectItem(data, "who");  if(!who)    goto err1;
        cJSON *what = cJSON_GetObjectItem(data, "what"); if(!what)   goto err1;
        switch(type->valueint)
        {
        case TYPE_DEV:; // { id: '', who: 'name'|'key', what: ''|{ name:'', value: ...}}
            DevConfig_t *dev = seachOneByRequired(devlist_head, 
                                (required_callback)isDevId, id->valuestring);
            if(!dev)    goto err1;
            if(!strcmp(who->valuestring, "name")) // who == "name"
            {
                strncpy(dev->name, what->valuestring, NAME_LEN);
                sendDataToWeb(PACK_DEV, dev);
            }
            else if(!strcmp(who->valuestring, "key")) // who == "key"
            {
                cJSON *keyname = cJSON_GetObjectItem(what, "name"); if(!keyname) goto err1;
                _key_t *key = seachOneByRequired(dev->keyList_head, 
                (required_callback)isSameKeyName, keyname->valuestring);
                if(!key)    goto err1;
                cJSON *keyvalue = cJSON_GetObjectItem(what, "value"); if(!keyvalue) goto err1;
                //backup keyvalue
                void *backup = malloc(sizeof(key->value));
                memcpy(backup, &key->value, sizeof(key->value));
                switch(key->type)
                {
                case KEY_BOOL: key->value.bool_ = cJSON_IsTrue(keyvalue); break;
                case KEY_NUMBER: key->value.number_ = keyvalue->valuedouble; break;
                case KEY_STRING: strncpy(key->value.string_, keyvalue->valuestring, KEY_LEN); break;
                case KEY_RANGE: key->value.range_.num = keyvalue->valuedouble; break;
                default: break;
                }
                char *buf = malloc(BUF_LEN);
                uint32_t len = dev_makeData(buf, key);
                EventConfig_t *evt = (EventConfig_t *)dev->ep_event;
                evt->buflen = dev_enPackage(buf, len, evt->buf, BUF_LEN, rand);
                _switchEventMode(evt, EPOLLOUT);
                memcpy(&key->value, backup, sizeof(key->value));
                free(backup);
                free(buf);
            }
            break;
        case TYPE_ROOM:;
            room_t *room = seachOneByRequired(roomlist_head, 
                    (required_callback)ext_isId, &id->valueint);
            if(room == NULL) // 添加房间 { id: -1, who: 'add', what: 'name'}
            {
                if(!strcmp(who->valuestring, "add")) // who == "name"
                {
                    room_t *newroom = initRoom();
                    if(newroom == NULL)    goto err1;
                    strncpy(newroom->base.name, what->valuestring, NAME_LEN);
                    newroom->base.id = ext_getNewId(roomlist_head);
                    appendTailList(&roomlist_head, newroom);
                    sendDataToWeb(PACK_ALL, NULL);
                }
                else goto err1;
            }
            if(!strcmp(who->valuestring, "name")) // who == "name"
            {
                strncpy(room->base.name, what->valuestring, NAME_LEN);
                sendDataToWeb(PACK_ROOM, room);
            }
            else if(!strcmp(who->valuestring, "adddev"))
            {
                room_dev_t *room_dev = initRoomDev();
                if(room_dev == NULL)    goto err1;
                strncpy(room_dev->id, what->valuestring, ID_LEN);
                appendList(&room->roomDevList_head, room_dev);
                registerRoomDev(room_dev, room);
                sendDataToWeb(PACK_ROOM, room);
            }
            else if(!strcmp(who->valuestring, "deldev"))
            {
                deleteNode(&room->roomDevList_head, (required_callback)isRoomDevId, 
                        what->valuestring, (destory_callback)destoryRoomDev);
                sendDataToWeb(PACK_ROOM, room);
            }
            else if(!strcmp(who->valuestring, "del"))
            {
                deleteNode(&roomlist_head, (required_callback)ext_isId, 
                    &room->base.id, (destory_callback)destoryRoom);
                int tmp = -1;
                travelList(roomlist_head, (manipulate_callback)ext_sortId_cb, &tmp);
                sendDataToWeb(PACK_ALL, NULL);
            }
            break;
        case TYPE_TODO:
            break;
        case TYPE_SET:;
            task_set_t *set = seachOneByRequired(setlist_head, 
                    (required_callback)ext_isId, &id->valueint);
            if(set == NULL) // 添加Set { id: -1, who: 'add', what: 'name'}
            {
                if(!strcmp(who->valuestring, "add")) // who == "name"
                {
                    task_set_t *newset = initTaskSet();
                    if(newset == NULL)    goto err1;
                    strncpy(newset->base.name, what->valuestring, NAME_LEN);
                    newset->base.id = ext_getNewId(setlist_head);
                    appendTailList(&setlist_head, newset);
                    sendDataToWeb(PACK_ALL, NULL);
                }
                else goto err1;
            }
            if(!strcmp(who->valuestring, "name"))
            {
                strncpy(set->base.name, what->valuestring, NAME_LEN);
                sendDataToWeb(PACK_SET, set);
            }
            else if(!strcmp(who->valuestring, "adddev")) // { "id": x, "who": "adddev", "what": {id: x, key: {...}}}
            {
                task_dev_t *task_dev = initTaskDev();
                if(task_dev == NULL)    goto err1;
                task_dev->base.id = ext_getNewId(set->task_devList_head);
                if(!getKeyByJson(&task_dev->key, what))
                {
                    free(task_dev);
                    goto err1;
                }
                appendList(&set->task_devList_head, task_dev);
                sendDataToWeb(PACK_SET, set);
            }
            else if(!strcmp(who->valuestring, "del"))
            {
                deleteNode(&setlist_head, (required_callback)ext_isId, 
                    &set->base.id, (destory_callback)destoryTaskSet);
                int tmp = -1;
                travelList(setlist_head, (manipulate_callback)ext_sortId_cb, &tmp);
                sendDataToWeb(PACK_ALL, NULL);
            }
            break;
        default:    break;
        }
    }
    cJSON_Delete(root);
    return ret;
err1:
    cJSON_Delete(root);
err:
#ifdef DEBUG_WEB
    fprintf(stderr, "<json_analysis>data error.\n");
#endif //DEBUG_WEB
    return 0;
}

char *pack_typeToString(uint8_t type)
{
    char *types[] = {
        "dev",
        "todo",
        "set",
        "room",
        "webConfig",
        "all",
    };
    if(type > sizeof(types)/sizeof(types[0]))   return NULL;
    return types[type];
}

void pack_addDev(DevConfig_t *devConfig, cJSON *data)
{
    if(!data || !devConfig)    return;
    cJSON_AddStringToObject(data, "name", devConfig->name);
    cJSON_AddStringToObject(data, "id", devConfig->id);
    cJSON_AddBoolToObject(data, "online", devConfig->isOnline);
    cJSON_AddNumberToObject(data, "type", devConfig->type);
    if(devConfig->room) cJSON_AddNumberToObject(data, "roomid", devConfig->room->base.id);
    else cJSON_AddNumberToObject(data, "roomid", -1);
    cJSON *keylist = cJSON_AddArrayToObject(data, "keylist");
    travelList(devConfig->keyList_head, (manipulate_callback)addKeyToArray, keylist);
}

void pack_addTodo(todo_t *todo, cJSON *data)
{
    if(!data || !todo)   return;
    cJSON_AddStringToObject(data, "name", todo->base.name);
    cJSON_AddNumberToObject(data, "id", todo->base.id);
    cJSON_AddNumberToObject(data, "set_id", todo->set_id);
    cJSON_AddNumberToObject(data, "con_type", todo->condition.type);
    cJSON *condition = cJSON_AddObjectToObject(data, "condition");
    if(todo->condition.type == CON_SENSOR)
    {
        con_sensor_t *con_sensor = &todo->condition.detail.con_sensor;
        cJSON_AddStringToObject(condition, "id", con_sensor->id);
        cJSON_AddNumberToObject(condition, "symbol", con_sensor->symbol);
        cJSON *keyobj = cJSON_AddObjectToObject(condition, "key");
        addKeyToObj(&con_sensor->key, keyobj, false);
    }
    else
    {
        //char buf[8];
        con_time_t *con_time = &todo->condition.detail.con_time;
        cJSON_AddStringToObject(condition, "time", con_time->time);
        cJSON_AddNumberToObject(condition, "days", con_time->days);
    }
}

void pack_addSet(task_set_t *set, cJSON *data)
{
    if(!data || !set)   return;
    cJSON_AddNumberToObject(data, "id", set->base.id);
    cJSON_AddStringToObject(data, "name", set->base.name);
    cJSON *tasklist = cJSON_AddArrayToObject(data, "tasklist");
    travelList(set->task_devList_head, (manipulate_callback)addTaskToArray, tasklist);
}

void pack_addRoom(room_t *room, cJSON *data)
{
    if(!data || !room)   return;
    cJSON_AddNumberToObject(data, "id", room->base.id);
    cJSON_AddStringToObject(data, "name", room->base.name);
    cJSON *devlist = cJSON_AddArrayToObject(data, "devlist");
    travelList(room->roomDevList_head, (manipulate_callback)addRoomDevIdToArray, devlist);
}

void pack_addWebConfig(WebConfig_t *webConfig, cJSON *data)
{
    if(!data || !webConfig)   return;
    cJSON_AddNumberToObject(data, "web_port", webConfig->web_port);
    cJSON_AddNumberToObject(data, "web6_port", webConfig->web6_port);
    cJSON_AddNumberToObject(data, "dev_port", webConfig->dev_port);
    cJSON_AddNumberToObject(data, "dev6_port", webConfig->dev6_port);
}

void pack_addAll(cJSON *data)
{
    char *listname[] = {
        "devlist",
        "todolist",
        "setlist",
        "roomlist"
    };
    node_t *heads[] = {
        devlist_head,
        todolist_head,
        setlist_head,
        roomlist_head
    };
    manipulate_callback call_back[] = {
        (manipulate_callback)addDevToArray,
        (manipulate_callback)addTodoToArray,
        (manipulate_callback)addSetToArray,
        (manipulate_callback)addRoomToArray
    };
    cJSON *list;
    int i;
    for(i = 0; i < 4; i++)
    {
        travelList(heads[i], call_back[i], cJSON_AddArrayToObject(data, listname[i]));
    }
}

void addKeyToArray(_key_t *key, cJSON *array)
{
    if(!key || !array)  return;
    cJSON *keyobj;
    cJSON_AddItemToArray(array, keyobj = cJSON_CreateObject());
    addKeyToObj(key, keyobj, true);
}

void addKeyToObj(_key_t *key, cJSON *obj, bool packModeAndUnit)
{
    if(!key || !obj)    return;
    cJSON_AddStringToObject(obj, "name", key->name);
    cJSON_AddNumberToObject(obj, "type", key->type);
    if(packModeAndUnit)
    {
        cJSON_AddNumberToObject(obj, "mode", key->mode);
        cJSON_AddStringToObject(obj, "unit", key->unit);
    }
    switch(key->type)
    {
    case KEY_BOOL: cJSON_AddBoolToObject(obj, "value", key->value.bool_); break;
    case KEY_STRING: cJSON_AddStringToObject(obj, "value", key->value.string_); break;
    case KEY_NUMBER: cJSON_AddNumberToObject(obj, "value", key->value.number_); break;
    case KEY_RANGE: 
        cJSON_AddNumberToObject(obj, "value", key->value.range_.num);
        cJSON_AddNumberToObject(obj, "top", key->value.range_.top);
        cJSON_AddNumberToObject(obj, "btn", key->value.range_.btn);
        cJSON_AddNumberToObject(obj, "step", key->value.range_.step);
        break;
    }
}

void addTaskToArray(task_dev_t *task, cJSON *array)
{
    if(!task || !array) return;
    cJSON *taskobj;
    cJSON_AddItemToArray(array, taskobj = cJSON_CreateObject());
    cJSON_AddNumberToObject(taskobj, "idx", task->base.id);
    cJSON_AddStringToObject(taskobj, "devid", task->devid);
    cJSON *keyobj = cJSON_AddObjectToObject(taskobj, "key");
    addKeyToObj(&task->key, keyobj, false);
}

void addRoomDevIdToArray(room_dev_t *room_dev, cJSON *array)
{
    if(!room_dev || !array)  return;
    cJSON_AddItemToArray(array, cJSON_CreateString(room_dev->id));
}

void addDevToArray(DevConfig_t *dev, cJSON *array)
{
    if(!dev || !array)  return;
    cJSON *devobj;
    cJSON_AddItemToArray(array, devobj = cJSON_CreateObject());
    pack_addDev(dev, devobj);
}

void addTodoToArray(todo_t *todo, cJSON *array)
{
    if(!todo || !array)  return;
    cJSON *todoobj;
    cJSON_AddItemToArray(array, todoobj = cJSON_CreateObject());
    pack_addTodo(todo, todoobj);
}

void addSetToArray(task_set_t *set, cJSON *array)
{
    if(!set || !array)  return;
    cJSON *setobj;
    cJSON_AddItemToArray(array, setobj = cJSON_CreateObject());
    pack_addSet(set, setobj);
}

void addRoomToArray(room_t *room, cJSON *array)
{    
    if(!room || !array)  return;
    cJSON *roomobj;
    cJSON_AddItemToArray(array, roomobj = cJSON_CreateObject());
    pack_addRoom(room, roomobj);
}

int getKeyByJson(_key_t *key, cJSON *what)
{
    cJSON *json = cJSON_GetObjectItem(what, "key");
    if(key == NULL) return -1;
    cJSON *name = cJSON_GetObjectItem(json, "name"); if(name == NULL)   return -1;
    strncpy(key->name, name->valuestring, NAME_LEN);
    cJSON *type = cJSON_GetObjectItem(json, "type");    if(type == NULL)    return -1;
    key->type = type->valueint;
    cJSON *value = cJSON_GetObjectItem(json, "value");
    if(value == NULL)   return -1;
    switch(key->type)
    {
    case KEY_BOOL: key->value.bool_ = value->valueint; break;
    case KEY_STRING: strncpy(key->value.string_, value->valuestring, KEY_LEN); break;
    case KEY_NUMBER: key->value.number_ = value->valuedouble; break;
    case KEY_RANGE:
        key->value.range_.num = value->valuedouble;
        char *strings[] = {
            "top",
            "btn",
            "step"
        };
        cJSON *obj[3];
        int i;
        for(i = 0; i < 3; i++)
        {
            obj[i] = cJSON_GetObjectItem(json, strings[i]);   if(obj[i] == NULL) return -1;
        }
        key->value.range_.top = obj[0]->valueint;
        key->value.range_.btn = obj[1]->valueint;
        key->value.range_.step = obj[2]->valuedouble;
        break;
    }
}