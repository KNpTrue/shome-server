/**
 * @shome-types.h
 * @brief type
*/

#ifndef _SHOME_TYPES_H
#define _SHOME_TYPES_H

#include <stdbool.h>

#ifdef SHOME_SERVER

#include <stdint.h>

#else //other

typedef signed char		int8_t;
typedef short int		int16_t;
typedef long int        int32_t;
typedef long long	    int64_t;

typedef unsigned char		uint8_t;
typedef unsigned short int	uint16_t;
typedef unsigned long  int  uint32_t;
typedef unsigned long long	uint64_t;

#endif //SHOME_SERVER

#ifndef NULL
#define NULL ((void*)0)
#endif //NULL

//size_t
typedef unsigned long len_t;

//生成随机数函数指针
typedef int (*rand_callback_t)( void );

//malloc
typedef void *(*malloc_cb)(len_t size);

//free
typedef void (*free_cb)(void *ptr);

#endif //_SHOME_TYPES_H