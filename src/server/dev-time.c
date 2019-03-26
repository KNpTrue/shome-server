/**
 * dev-time.c
 * 定时任务的实现
 */
#include "dev-time.h"
#include <time.h>
#include <string.h>
#include <stdio.h>
#include <sys/time.h>

static struct next_alarm {
    time_t     alarm; // 距离下一次闹钟的时间
    node_t     *setlist_head; // 下一个时钟到的时候会触发该set链表中的事件
} next = {.alarm = 0, .setlist_head = NULL};

/**
 * 检查是不是当天要执行todo任务
 * @wday: 当天是星期几
*/
bool isDayTodo(con_time_t *con_time, int wday); // wday 0~6
/**
 * 将con_time中的时间字符串转换成tm结构体中的时间
 * @return: true成功, false则表示con_time中的时间格式错误
*/
bool setHourMin(con_time_t *con_time, struct tm *tm);
/**
 * 是不是闰年
*/
bool isLeapYear(int year);

void updateNextAlarm(todo_t *todo)
{
    uint8_t mdays[] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}; //月份天数
    if(todo->condition.type == CON_SENSOR)  return;
    con_time_t *con_time = &todo->condition.detail.con_time;
    time_t now_t = time(NULL);
    struct tm *now = localtime(&now_t), tmp;
    if(isLeapYear(now->tm_year + 1900)) mdays[2] = 29;
    memcpy(&tmp, now, sizeof(struct tm));
    tmp.tm_sec = 0;
    int i;
    if(!setHourMin(con_time, &tmp)) return; //change hour min
    for(i = 0;; i++)
    {
        tmp.tm_wday = (now->tm_wday + i) % 7;
        if(isDayTodo(con_time, tmp.tm_wday))    break;
    }
    if(tmp.tm_wday == now->tm_wday && (now->tm_hour > tmp.tm_hour ||
        (now->tm_hour == tmp.tm_hour && now->tm_min >= tmp.tm_min))) //当天且在之前
    {
        for(i = 1;; i++)
        {
            tmp.tm_wday = (now->tm_wday + i) % 7;
            if(isDayTodo(con_time, tmp.tm_wday))    break;
        }
    }
    if(tmp.tm_mday + i > mdays[tmp.tm_mon]) //change year mon mday
    {
        tmp.tm_mday = i - (mdays[tmp.tm_mon] - tmp.tm_mday);
        tmp.tm_mon = (tmp.tm_mon + 1) % 12;
        if(tmp.tm_mon == 0)
        {
            tmp.tm_yday = tmp.tm_mday;
            tmp.tm_year += 1;
        }
        else tmp.tm_yday += i;
    }
    else tmp.tm_mday += i;
    time_t next_t = mktime(&tmp);
    if(next_t <= now_t)  return;
    if(next_t > next.alarm && next.alarm != 0) return;
    else if(next_t < next.alarm)        
        deleteList(&next.setlist_head, NULL);
    //update
    next.alarm = next_t;
    appendList(&next.setlist_head, todo->set);
#ifdef DEBUG_DEV
    printf("next_t:%u alarm:%u next: %d-%d-%d %d:%d\n", next_t, next.alarm, tmp.tm_year + 1900, 
        tmp.tm_mon + 1, tmp.tm_mday, tmp.tm_hour, tmp.tm_min);
#endif //DEBUG_DEV
}

void setNextAlarm()
{
    struct itimerval new;
    memset(&new, 0, sizeof(new));
    new.it_value.tv_sec = next.alarm - time(NULL);
    setitimer(ITIMER_REAL, &new, NULL);
}

node_t *getNextSetList()
{
    return next.setlist_head;
}

void resetAlarm()
{
    deleteList(&next.setlist_head, NULL);
    next.alarm = 0;
}

bool isDayTodo(con_time_t *con_time, int wday)
{
    return con_time->days & (1 << wday);
}

bool setHourMin(con_time_t *con_time, struct tm *tm)
{
    con_time->time[2] = '\0';
    tm->tm_hour = atoi(con_time->time);
    if(tm->tm_hour > 23 || tm->tm_hour < 0) return false;
    tm->tm_min = atoi(con_time->time + 3);
    if(tm->tm_min > 59 || tm->tm_min < 0)   return false;
    con_time->time[2] = ':';
    return true;
}

bool isLeapYear(int year)
{
    return ((year % 4 == 0 && year % 100 != 0) || year % 400==0);
}