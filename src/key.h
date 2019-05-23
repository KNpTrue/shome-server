//key.h
#ifndef _KEY_H
#define _KEY_H

#include "shome-types.h"

//用户必须实现malloc free !!
extern malloc_cb key_malloc;
extern free_cb key_free;

#define  KEY_LEN  32
#define  UNIT_LEN 8
#define  KEY_COUNT_BYTE 1 

enum _symbol {
    SYM_EQUAL,   //=
    SYM_GREATER, //>
    SYM_LESS,    //<
};

enum _keytype {
    KEY_NUMBER,
    KEY_STRING,
    KEY_BOOL,
    KEY_RANGE,
};

enum _keymode {
    KEY_READONLY,
    KEY_READWRITE,
};
//根据enum获取各种属性的字符串
enum string_type {
    KEY_TYPE,
    KEY_MODE,
    KEY_SYMOL
};
char **getKeyTypesString(uint8_t type);
//有上下限的key类型
typedef struct range{
    float num;
    float step;
    int32_t top;
    int32_t btn;
} range_t;

//key
typedef struct key {
    char        name[KEY_LEN];
    uint8_t     type;
    uint8_t     mode; //r rw
    char        unit[UNIT_LEN]; //单位
    union {
        double      number_;
        char        string_[KEY_LEN];
        bool        bool_;
        range_t     range_;
    }           value;
} _key_t;

//init
bool initKey(_key_t *key, char *name, uint8_t type, uint8_t mode, char *unit);
//打印键值
void printKey(_key_t *key);
int sprintKeyValue(char *buf, _key_t *key);
//设置key且偏移buf指针
void setKeyValue(_key_t *key, const void *buf);
void setKeyValue_move(_key_t *key, const void **buf);
//安全的指针偏移
struct safa_data {
    uint32_t buflen;
    void     *buf;
};
bool setKeyValue_move_safe(_key_t *key, struct safa_data *data);
//将值放入buf并偏移buf指针
uint32_t valueToBuf(_key_t *key, void **buf);
//通过string设置keyvalue
void setKeyValueByStr(_key_t *key, const char *buf);
//是否为相同的name
bool isSameKeyName(_key_t *key, const char *keyName);
//key是否符合条件
bool isKeyRequired(_key_t *srckey, _key_t *req, uint8_t symbol);
//获得值的大小
uint32_t getValueSize(uint8_t type);
//拷贝key不包括key的部分
void copyKeyHead_move(_key_t *key, void **buf);
#endif //_KEY_H