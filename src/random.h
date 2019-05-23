#ifndef _RANDOM_H
#define _RANDOM_H

#include "shome-types.h"

/**
 * 获得一个随机字符串
*/
void getRandomString(uint8_t *str, uint32_t strlen, rand_callback_t cb);

#endif //_RANDOM_H