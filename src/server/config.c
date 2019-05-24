//config.c
#include "config.h"
#include "dev.h"
#include "web-config.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define    LINE_MAX   256
//head
enum head_idx {
    HEAD_WEB,
    HEAD_DEV,
    HEAD_TODO,
    HEAD_SET,
    HEAD_ROOM
};
//type
static char *head_type[] = {
    "web",
    "dev",
    "todo",
    "set",
    "room",
    NULL
};
static char *condition_type[] = {
    "sensor",
    "time",
    NULL
};
//bool
static char *boolstr[] = {
    "false",
    "true",
    NULL
};

enum body_who {
    BODY_WEB,
    BODY_DEV,
    BODY_SET,
    BODY_TASK,
    BODY_KEY,
    BODY_TODO,
    BODY_CONDITION,
    BODY_ROOM
};
//初始化 返回文件流指针
FILE *initConfFile(char *mode);
//找到head  id
int findHeadConf(FILE *fp, uint8_t *head_idx, char *id);
//找到对应head的body
int findBodyConf(FILE *fp, void *tag, uint8_t who);
//获取key值 "key value"
void readValue_move(char *dst, uint32_t dstlen, const char *src, uint32_t offset);
double readValueNum_move(const char *src, uint32_t offset);
uint8_t readValueIdx_move(char **valueArrary, const char *src, uint32_t offset);
//write
void writeWebConf(WebConfig_t *webConfig, FILE *fp);
void writeDevConf(DevConfig_t *dev, FILE *fp);
void writeToDoConf(todo_t *todo, FILE *fp);
void writeSetConf(task_set_t *set, FILE *fp);
void writeTaskConf(task_dev_t *task, FILE *fp);
void writeRoomConf(room_t *room, FILE *fp);
void writeDevId(room_dev_t *room_dev, FILE *fp);

int readConf()
{
    WebConfig_t *webConfig = getWebConfig();
    FILE *fp = initConfFile("r");
    if(fp == NULL)  return -1;
    char        buf[LINE_MAX];
    uint32_t    buflen;
    uint8_t     head_idx;
    char        id[ID_LEN + 1];
    uint8_t     id_num;
    DevConfig_t *dev;
    task_set_t  *set;
    todo_t      *todo;
    room_t      *room;
    while(1)
    {
        if(findHeadConf(fp, &head_idx, id)) break;
        switch(head_idx)
        {
        case HEAD_WEB: //web相关配置
            if(findBodyConf(fp, webConfig, BODY_WEB))   goto err;
            break;
        case HEAD_DEV:
            if(strlen(id) != ID_LEN)    goto err1;
            if(devlist_head) 
                if(seachOneByRequired(devlist_head, (required_callback)isDevId, id))
                    goto err1;
            dev = initDevConfig();
            if(dev == NULL) goto err2;
            strncpy(dev->id, id, ID_LEN);
            appendTailList(&devlist_head, dev);
            if(findBodyConf(fp, dev, BODY_DEV))    goto err1;
            break;
        case HEAD_SET:
            id_num = atoi(id);
            if(setlist_head)
                if(seachOneByRequired(setlist_head, (required_callback)ext_isId, &id_num))
                    goto err1;
            set = initTaskSet();
            if(set == NULL) goto err2;
            set->base.id = id_num;
            appendTailList(&setlist_head, &set->base);
            if(findBodyConf(fp, set, BODY_SET))    goto err1;
            break;
        case HEAD_TODO:
            id_num = atoi(id);
            if(todolist_head)
                if(seachOneByRequired(todolist_head, (required_callback)ext_isId, &id_num))
                    goto err1;
            todo = malloc(sizeof(todo_t));
            if(todo == NULL)    goto err2;
            todo->base.id = atoi(id);
            appendTailList(&todolist_head, todo);
            if(findBodyConf(fp, todo, BODY_TODO))   goto err1;
            break;
        case HEAD_ROOM:
            id_num = atoi(id);
            if(roomlist_head)
                if(seachOneByRequired(roomlist_head, (required_callback)ext_isId, &id_num))
                    goto err1;
            room = initRoom();
            if(room == NULL)    goto err2;
            room->base.id = id_num;
            appendTailList(&roomlist_head, room);
            if(findBodyConf(fp, room, BODY_ROOM))   goto err1;
            break;
        }
    }
    fclose(fp);
    return 0;
err2:
#ifdef DEBUG_DEV
    fprintf(stderr, "<findDevConf>init error.\n");
#endif //DEBUG_DEV
    goto err;
err1:
#ifdef DEBUG_DEV
    fprintf(stderr, "<findDevConf>Config error, please check the config.\n");
#endif //DEBUG_DEV
    goto err;
err:
    fclose(fp);
    return -1;
}

int writeConf()
{
    FILE *fp = initConfFile("w");
    if(fp == NULL)  return -1;
    //web
    writeWebConf(getWebConfig(), fp);
    //dev
    travelList(devlist_head, (manipulate_callback)writeDevConf, fp);
    //todo
    travelList(todolist_head, (manipulate_callback)writeToDoConf, fp);
    //set
    travelList(setlist_head, (manipulate_callback)writeSetConf, fp);
    //room
    travelList(roomlist_head, (manipulate_callback)writeRoomConf, fp);
    fclose(fp);
    return 0;
}

int findBodyConf(FILE *fp, void *tag, uint8_t who)
{
    char buf[LINE_MAX], *p, *t, buf2[LINE_MAX];
    uint32_t    buflen;
    while(1)
    {
        if(!fgets(buf, LINE_MAX, fp)) return -1;
        buflen = strlen(buf);
        if(strchr(buf, '}'))     break;
        p = buf;
        switch(who)
        {
        case BODY_WEB:;
            WebConfig_t *webConfig = tag;
            if((t = strstr(p, "web_port")) != NULL)
            {
                readValue_move(buf2, sizeof(buf2), t, 8);
                webConfig->web_port = atoi(buf2);
            }
            else if((t = strstr(p, "web6_port")) != NULL)
            {
                readValue_move(buf2, sizeof(buf2), t, 9);
                webConfig->web6_port = atoi(buf2);
            }
            else if((t = strstr(p, "dev_port")) != NULL)
            {
                readValue_move(buf2, sizeof(buf2), t, 8);
                webConfig->dev_port = atoi(buf2);
            }
            else if((t = strstr(p, "dev6_port")) != NULL)
            {
                readValue_move(buf2, sizeof(buf2), t, 9);
                webConfig->dev6_port = atoi(buf2);
            }
            else if((t = strstr(p, "passwd")) != NULL)
            {
                readValue_move(webConfig->passwd, PASSWD_LEN, t, 6);
            }
        case BODY_DEV:;
            DevConfig_t *devConfig = tag;
            if((t = strstr(p, "name")) != NULL)
            {
                readValue_move(devConfig->name, NAME_LEN, t, 4);
            }
            else if((t = strstr(p, "type")) != NULL)
            {
                devConfig->type = readValueNum_move(t, 4);
            }
            break;
        case BODY_SET:;
            task_set_t *set = tag;
            if((t = strstr(p, "name")) != NULL)
            {
                readValue_move(set->base.name, NAME_LEN, t, 4);
            }
            else if((t = strstr(p, "task")) != NULL) //task
            {
                if(!fgets(buf, LINE_MAX, fp)) return -1;
                if(!strchr(buf, '{'))   return -1;
                task_dev_t *task = initTaskDev();
                if(task == NULL)
                {
#ifdef DEBUG_DEV
                    fprintf(stderr, "<findBodyConf>malloc error.\n");
#endif //DEBUG_DEV
                    return -1;
                }
                appendTailList(&set->task_devList_head, task);
                if(findBodyConf(fp, task, BODY_TASK))  return -1;
            }
            break;
        case BODY_TASK:;
            task_dev_t *task = tag;
            if((t = strstr(p, "idx")) != NULL)
            {
                task->base.id = readValueNum_move(t, 3);
            }
            else if((t = strstr(p, "devid")) != NULL)
            {
                memset(task->devid, 0, ID_LEN + 1);
                readValue_move(task->devid, ID_LEN, t, 5);
            }
            else if((t = strstr(p, "key")) != NULL)
            {
                task->key.type = readValueIdx_move(getKeyTypesString(KEY_TYPE), t, 3);
                if(!fgets(buf, LINE_MAX, fp)) return -1;
                if(!strchr(buf, '{'))   return -1;
                if(findBodyConf(fp, &task->key, BODY_KEY)) return -1;
            }
            break;
        case BODY_KEY:;
            _key_t *key = tag;
            if((t = strstr(p, "name")) != NULL)
            {
                readValue_move(key->name, KEY_LEN, t, 4);
            }
            else if((t = strstr(p, "value")) != NULL)
            {
                readValue_move(buf2, LINE_MAX, t, 5);
                setKeyValueByStr(key, buf2);
            }
            break;
        case BODY_TODO:;
            todo_t *todo =tag;
            todo->set = NULL;
            if((t = strstr(p, "name")) != NULL)
            {
                readValue_move(todo->base.name, NAME_LEN, t, 4);
            }
            else if((t = strstr(p, "condition")) != NULL)
            {
                todo->condition.type = readValueIdx_move(condition_type, t, 9);
                if(!fgets(buf, LINE_MAX, fp)) return -1;
                if(!strchr(buf, '{'))   return -1;
                
                if(findBodyConf(fp, &todo->condition, BODY_CONDITION)) 
                    return -1;
                
            }
            else if((t = strstr(p, "task_set_id")) != NULL)
            {
                //id
                readValue_move(buf2, sizeof(buf2), t, 11);
                todo->set_id = atoi(buf2);
            }
            break;
        case BODY_CONDITION:;
            condition_t *condition = tag;
            switch(condition->type)
            {
            case CON_SENSOR:
                condition->detail.con_sensor.devConfig = NULL;
                condition->detail.con_sensor.flag = false;
                if((t = strstr(p, "id")) != NULL)
                {
                    memset(condition->detail.con_sensor.id, 0, ID_LEN + 1);
                    readValue_move(condition->detail.con_sensor.id, ID_LEN, t, 2);
                }
                else if((t = strstr(p, "symbol")) != NULL)
                {
                    condition->detail.con_sensor.symbol = readValueIdx_move(getKeyTypesString(KEY_SYMOL), t, 6);
                }
                else if((t = strstr(p, "key")) != NULL)
                {
                    condition->detail.con_sensor.key.type = readValueIdx_move(getKeyTypesString(KEY_TYPE), t, 3);
                    printf("type: %d\n", condition->detail.con_sensor.key.type);
                    if(!fgets(buf, LINE_MAX, fp)) return -1;
                    if(!strchr(buf, '{'))   return -1;
                    if(findBodyConf(fp, &condition->detail.con_sensor.key, BODY_KEY))
                        return -1;
                }
                break;
            case CON_TIME:
                if((t = strstr(p, "time")) != NULL)
                {
                    readValue_move(condition->detail.con_time.time, TIME_LEN, t, 4);
                }
                else if((t = strstr(p, "days")) != NULL)
                {
                    char buf2[8];
                    readValue_move(buf2, sizeof(buf2), t, 4);
                    setDays(&condition->detail.con_time, buf2);
                }
                break;
            }
            break;
        case BODY_ROOM:;
            room_t *room = tag;
            if((t = strstr(p, "name")) != NULL)
            {
                readValue_move(room->base.name, KEY_LEN, t, 4);
            }
            else if((t = strstr(p, "dev_id")) != NULL)
            {
                room_dev_t *room_dev = initRoomDev();
                if(room_dev == NULL)    return -1;
                readValue_move(room_dev->id, ID_LEN + 1, t, 6);
                appendTailList(&room->roomDevList_head, room_dev);
            }
            break;
        }
    }
    return 0;
}

FILE *initConfFile(char *mode)
{
    FILE *fp = fopen(DEV_CONF_PATH, mode);
    if(fp == NULL)
    {
#ifdef DEBUG_DEV
        perror(DEV_CONF_PATH);
#endif //DEBUG_DEV
        return NULL;
    }
    return fp;
}

int findHeadConf(FILE *fp, uint8_t *head_idx, char *id)
{
    char buf[LINE_MAX];
    memset(buf, 0, LINE_MAX);
    uint32_t buflen;
    uint8_t  i;
    char *p;
    while(1)
    {
        if(!fgets(buf, LINE_MAX, fp))   break;
        buflen = strlen(buf);
        if(buflen == 1) continue; //说明是空行
        buf[buflen - 1] = '\0';
        p = buf;
        while(*p == ' ') p++; //去掉空格
        for(i = 0; head_type[i] != NULL; i++)
        {
            if(!strncmp(p, head_type[i], strlen(head_type[i]))) //匹配字符
            {
                *head_idx = i;
                //id
                p += strlen(head_type[i]);
                while(*p == ' ') p++; //去掉空格
                strcpy(id, p);
                if(!fgets(buf, LINE_MAX, fp))   break;
                if(strchr(buf, '{'))   return 0;
                else            return -1;
            }
        }
    }
    return -1;
}

void readValue_move(char *dst, uint32_t dstlen, const char *src, uint32_t offset)
{
    src += offset;
    while(*src == ' ' || *src == '=') src++;
    if(*src == '"') src++;
    while(*src !='"' && dstlen > 0 && *src != '\0')
    {
        *dst++ = *src++;
        dstlen--;
    }
    if(dstlen != 0) *dst = '\0';
}

double readValueNum_move(const char *src, uint32_t offset)
{
    char buf[LINE_MAX];
    readValue_move(buf, LINE_MAX, src, offset);
    return atof(buf);
}

uint8_t readValueIdx_move(char **valueArrary, const char *src, uint32_t offset)
{
    src += offset;
    while(*src == ' ' || *src == '=') src++;
    if(*src == '"') src++;
    uint8_t i;
    char buf[KEY_LEN], *p = buf;
    while(*src !='"' && *src != '\0')   *p++ = *src++;
    *p = '\0';
    for(i = 0; valueArrary[i] != NULL; i++)
    {
        if(!strncmp(buf, valueArrary[i], strlen(valueArrary[i])))
            return i;
    }
    return -1;
}
void writeWebConf(WebConfig_t *webConfig, FILE *fp)
{
    fprintf(fp, "web\n{\n    web_port = \"%d\"\n    web6_port = \"%d\"\n    dev_port = \"%d\"\n    dev6_port = \"%d\"\n    passwd = \"%s\"\n}\n\n", 
            webConfig->web_port, webConfig->web6_port, webConfig->dev_port, webConfig->dev6_port, webConfig->passwd);
}

void writeDevConf(DevConfig_t *dev, FILE *fp)
{
    fprintf(fp, "dev %s\n{\n    type = \"%d\"\n    name = \"%s\"\n}\n\n", 
            dev->id, dev->type, dev->name);
}

void writeToDoConf(todo_t *todo, FILE *fp)
{   
    char buf[KEY_LEN];
    fprintf(fp, "todo %d\n{\n    name = \"%s\"\n    condition %s\n    {\n", 
            todo->base.id, todo->base.name, condition_type[todo->condition.type]);
    switch(todo->condition.type)
    {
    case CON_SENSOR:;
        con_sensor_t *con_sensor = &todo->condition.detail.con_sensor;
        fprintf(fp, "        id = \"%s\"\n        key %s\n        {\n", 
                con_sensor->id, getKeyTypesString(KEY_TYPE)[con_sensor->key.type]);
        sprintKeyValue(buf, &con_sensor->key);
        fprintf(fp, "            name = \"%s\"\n            value = \"%s\"\n        }\n", 
                con_sensor->key.name, buf);
        fprintf(fp, "        symbol = \"%s\"\n", getKeyTypesString(KEY_SYMOL)[con_sensor->symbol]);
        break;
    case CON_TIME:;
        con_time_t *con_time = &todo->condition.detail.con_time;
        fprintf(fp, "        time = \"%s\"\n        days = \"%s\"\n", 
                con_time->time, daysToStr(con_time, buf, KEY_LEN));
        break;
    }
    fprintf(fp, "    }\n    task_set_id = \"%d\"\n}\n\n", todo->set_id);
}

void writeSetConf(task_set_t *set, FILE *fp)
{
    fprintf(fp, "set %d\n{\n    name = \"%s\"\n", set->base.id, set->base.name);
    travelList(set->task_devList_head, (manipulate_callback)writeTaskConf, fp);
    fprintf(fp, "}\n\n");
}

void writeTaskConf(task_dev_t *task, FILE *fp)
{
    char buf[KEY_LEN];
    sprintKeyValue(buf, &task->key);
    fprintf(fp, "    task\n    {\n        idx = \"%d\"\n        devid = \"%s\"\n        key %s\n        {\n", 
            task->base.id, task->devid, getKeyTypesString(KEY_TYPE)[task->key.type]);
    fprintf(fp, "            name = \"%s\"\n            value = \"%s\"\n        }\n    }\n", 
                task->key.name, buf);
}

void writeRoomConf(room_t *room, FILE *fp)
{
    fprintf(fp, "room %d\n{\n    name = \"%s\"\n", room->base.id, room->base.name);
    travelList(room->roomDevList_head, (manipulate_callback)writeDevId, fp);
    fprintf(fp, "}\n\n");
}

void writeDevId(room_dev_t *room_dev, FILE *fp)
{
    fprintf(fp, "    dev_id = \"%s\"\n", room_dev->id);
}