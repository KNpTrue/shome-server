/**
 * dev-time.h
 * todo任务中定时任务实现的方法，在main函数中被调用
 */
#ifndef _DEV_TIME_H
#define _DEV_TIME_H

#include "dev-ext.h"
#include "../list.h"

/**
 * 更新下一个闹钟
 * 通过遍历每个todo任务可以获取最新最近的闹钟事件
 * 它会更新内部的一个next_alarm结构体中的信息，
 * 然后再通过setNextAlarm更新系统的定时器
 */
void updateNextAlarm(todo_t *todo);
/**
 * 设置下一次的闹钟的定时器
 */
void setNextAlarm();
/**
 * 获得下一个定时任务的链表
 * @return: 返回一个数据为 task_set_t 的链表
 */
node_t *getNextSetList();
/**
 * 重置定时器
 * 它会将nextAlarm结构体中的数据全部清除，
 * 需要重新updateNextAlarm去获取下一次闹钟事件
 */
void resetAlarm();

#endif //_DEV_TIME_H