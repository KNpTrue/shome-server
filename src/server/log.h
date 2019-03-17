#ifndef _LOG_H
#define _LOG_H

#include <errno.h>
#include <stdio.h>

#define     LOG_PATH   "/var/log/SHome_Server.log"
#define     LOG_ERR(STR)    writelog(STR, strerror(errno))
#define     LOG(STR)            writelog(STR, NULL)

/*  writelog
 *  str1和str2写到文件中并添加上换行符
 */
void writelog(const char *str1, const char *str2);

#endif //_LOG_H