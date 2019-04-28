#ifndef _LOG_H
#define _LOG_H

#include <stdio.h>

#ifdef DEBUG
#define logd(fmt, a...) printf(fmt, ##a)
#define loge(fmt, a...) fprintf(stderr, fmt, ##a)
#define logp(str)   perror(str)
#else
#define logd(fmt, a...) do {} while (0)
#define loge(fmt, a...) do {} while (0)
#define logp(str)   do {} while (0)
#endif //DEBUG



#endif //_LOG_H