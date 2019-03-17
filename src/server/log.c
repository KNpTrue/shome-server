//log.c
#include "log.h"

#include <time.h>
#include <string.h>

void writelog(const char *str1, const char *str2)
{
    FILE *logfp = fopen(LOG_PATH, "a");
    time_t now;
    time(&now);// get now time
    char time_str[32];
    ctime_r(&now, time_str);
    int len = strlen(time_str);
    time_str[len - 1] = '\0';
    fwrite("[", 1, 1, logfp);
    if(time_str != NULL)
        fwrite(time_str, strlen(time_str), 1, logfp);
    fwrite("] ", 2, 1, logfp);
    if(str1 != NULL)
        fwrite(str1, strlen(str1), 1, logfp);
    if(str2 != NULL)
        fwrite(str2, strlen(str2), 1, logfp);
    fwrite("\n", 1, 1, logfp);
    fclose(logfp);
}