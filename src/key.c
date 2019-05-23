//key.c
#include "key.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static char *key_types[] = {
    "number",
    "string",
    "bool",
    "range",
    NULL
};

static char *key_modes[] = {
    "r",
    "rw",
    NULL
};

static char *key_symols[] = {
    "equal",
    "greater",
    "less",
    NULL
};

#define    ARR_NUM(a)    (sizeof(a)/sizeof(a[0]))
#define    offset_t(type, member)    (long)(&((type *)0)->member)
//isKeyRequired
#define    ISKEYREQUIRED(srckey, reqkey, type) \
            ({switch(symbol)\
            {\
            case SYM_EQUAL:\
                return (srckey)->value.type == (reqkey)->value.type;\
            case SYM_GREATER:\
                return (srckey)->value.type > (reqkey)->value.type;\
            case SYM_LESS:\
                return (srckey)->value.type < (reqkey)->value.type;\
            }})\

//float to double
void f2d( float f , void *x );

char **getKeyTypesString(uint8_t type)
{
    switch(type)
    {
    case KEY_TYPE: return key_types;
    case KEY_MODE: return key_modes;
    case KEY_SYMOL: return key_symols;
    }
    return NULL;
}

bool initKey(_key_t *key, char *name, uint8_t type, uint8_t mode, char *unit)
{
    if(key == NULL || name == NULL) return false;
    memset(key, 0, sizeof(_key_t));
    strncpy(key->name, name, KEY_LEN);
    key->type = type;
    if(type == KEY_RANGE)
    key->value.range_.step = 1;
    key->value.range_.btn = 0;
    key->value.range_.top = 100;
    key->mode = mode;
    if(unit)    strncpy(key->unit, unit, UNIT_LEN);
    return true;
}

int sprintKeyValue(char *buf, _key_t *key)
{
    switch(key->type)
    {
    case KEY_NUMBER:
        return sprintf(buf, "%f", key->value.number_);
    case KEY_STRING:
        return sprintf(buf, "%s", key->value.string_);
    case KEY_BOOL:
        return sprintf(buf, "%s", (key->value.bool_ ? "true" : "false"));
    case KEY_RANGE:
        return sprintf(buf, "%f", key->value.range_.num);
    }
    return -1;
}
void printKey(_key_t *key)
{
    char buf[KEY_LEN * 2];
    sprintKeyValue(buf, key);
    printf("%s:%s%s\n", key->name, buf, key->unit);
}

void setKeyValue(_key_t *key, const void *buf)
{
    memcpy(&key->value, buf, getValueSize(key->type));
}

void setKeyValue_move(_key_t *key, const void **buf)
{
    if(!key || !buf)    return;
    uint32_t size = getValueSize(key->type);
    memcpy(&key->value, *buf, size);
    *buf += size;
}

bool setKeyValue_move_safe(_key_t *key, struct safa_data *data)
{
    if(!key || !data)   return false;
    uint32_t size = getValueSize(key->type);
    if(data->buflen < size) return false;
    if(data->buflen < size)    return false;
    memcpy(&key->value, data->buf, size);
    data->buflen -= size;
    data->buf += size;
    return true;
}

uint32_t valueToBuf(_key_t *key, void **buf)
{
    if(!key || !buf)    return 0;
    uint32_t size = getValueSize(key->type);
    if(sizeof(double) == 4 && key->type == KEY_NUMBER)
    {
        char dnum[sizeof(double) * 2];
        f2d(key->value.number_, (void *)dnum);
        memcpy(*buf, dnum, size);
    }
    else memcpy(*buf, &key->value, size);
    *buf += size;
    return size;
}

void setKeyValueByStr(_key_t *key, const char *buf)
{
    switch(key->type)
    {
    case KEY_NUMBER:
        key->value.number_ = atof(buf);
        break;
    case KEY_STRING:
        strcpy(key->value.string_, buf);
        break;
    case KEY_BOOL:
        if(!strcmp(buf, "true"))
            key->value.bool_ = true;
        else key->value.bool_ = false;
        break;
    case KEY_RANGE:
        key->value.range_.num = atof(buf);
        break;
    }
}

bool isSameKeyName(_key_t *key, const char *keyName)
{
    return strncmp(key->name, keyName, KEY_LEN) == 0;
}

bool isKeyRequired(_key_t *srckey, _key_t *reqkey, uint8_t symbol)
{
    switch(srckey->type)
    {
    case KEY_BOOL:
        return srckey->value.bool_ == reqkey->value.bool_;
    case KEY_NUMBER:
        ISKEYREQUIRED(srckey, reqkey, number_);
    case KEY_RANGE:
        ISKEYREQUIRED(srckey, reqkey, range_.num);
    case KEY_STRING:
        return !strcmp(srckey->value.string_, reqkey->value.string_);
        break;
    }
    return false;
}

uint32_t getValueSize(uint8_t type)
{
    uint8_t value_size[] = {sizeof(double), KEY_LEN, 
                            sizeof(bool), sizeof(range_t)};
    if(sizeof(double) == 4) //保证数据统一
    {
        value_size[0] = sizeof(double) * 2;
    }
    uint8_t i;
    for(i = 0; i < ARR_NUM(value_size); i++)
    {
        if(type == i)  break;
    }
    return value_size[i];
}

void copyKeyHead_move(_key_t *key, void **buf)
{
    if(!key || !buf || !*buf)   return;
    uint32_t len = offset_t(_key_t, unit) + UNIT_LEN;
    memcpy(*buf, key, len);
    *buf += len;
}

void f2d( float f , void *x )
{
    unsigned long a , b;
    unsigned long uf = *(unsigned long*)&f;
    unsigned long*ux = (unsigned long*)x;
     
    ux[0] = ux[1] = 0;
    ux[1] |= uf&0x80000000;
     
    a = (uf&0x7f800000)>>23;
    b = uf&0x7fffff;
    a += 1024 - 128;
    ux[1] |= a<<20;
    ux[1] |= b>>3 ;
    ux[0] |= b<<29;   
}