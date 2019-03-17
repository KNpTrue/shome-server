//dev-time.h
#ifndef _DEV_TIME_H
#define _DEV_TIME_H

#include "dev-detail.h"
#include "../list.h"


//更新下一个闹钟
void updateNextAlarm(todo_t *todo);
//设置下一次的闹钟
void setNextAlarm();
//获得下一个定时任务
node_t *getNextSetList();
//销毁定时器
void resetAlarm();

#endif //_DEV_TIME_H