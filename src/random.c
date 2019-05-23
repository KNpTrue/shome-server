//random.c
#include "random.h"

#include <stdlib.h>

void getRandomString(uint8_t *str, uint32_t strlen, rand_callback_t cb)
{
    int i;
    for(i = 0; i < strlen; i++)
    {
        str[i] = (uint8_t)cb() % 256;
    }
}