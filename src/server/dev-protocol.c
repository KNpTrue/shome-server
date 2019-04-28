//dev-protocol.c

#include "dev-protocol.h"
#include <string.h>
#include <stdio.h>

#define  MASKEY_LEN  4

DevConfig_t *dev_handShake(const char *buf, uint32_t buflen)
{
    if(buf == NULL || buflen < 3 + ID_LEN)
    {
#ifdef DEBUG_DEV
        fprintf(stderr, "<initDev>parameter illegal.\n");
#endif //DEBUG_DEV
        return NULL;
    }
    if(strstr(buf, DEV_MAGIC) != buf) //握手协议头三个字节标识
    {
#ifdef DEBUG_DEV
        fprintf(stderr, "<initDev>protocol illegal.\n");
#endif //DEBUG_DEV
        return NULL;
    }
    char *p = (char *)buf;
    p += 3;
    //id
    char id[ID_LEN];
    int i;
    for(i = 0; i < ID_LEN; i++)
    {
        id[i] = *p++;
    }
    //type
    dev_type_t type;
    memcpy(&type, p, sizeof(dev_type_t));
    p += sizeof(dev_type_t);
    DevConfig_t *dev = seachOneByRequired(*getDevListHead(), 
                        (required_callback)isDevId, &id);
    if(!dev)
    {
        dev = initDevConfig();
        if(dev == NULL) return NULL;
        appendTailList(getDevListHead(), dev);
        strncpy(dev->id, id, ID_LEN);
        //name
        strcpy(dev->name, "new device");
    }
    else
    {
        if(dev->keyList_head)   deleteList(&dev->keyList_head, NULL);
        if(dev->ep_event != NULL)   return NULL;
    }
    dev->isOnline = true;
    dev->type = type;
    //keyList
    uint8_t keyNum = *p++;
    if(buflen < 3 + ID_LEN + sizeof(dev_type_t) + KEY_COUNT_BYTE + keyNum * (KEY_LEN + 1 + 1 + UNIT_LEN)) 
        goto err;
    //将key串成链表
    node_t *head = NULL;
    _key_t *key;
    for(i = 0; i < keyNum; i++)
    {
        key = malloc(sizeof(_key_t));
        if(key == NULL)
        {
            deleteList(&head, NULL);
            goto err;
        }
        strncpy(key->name, p, KEY_LEN);
        p += KEY_LEN;
        key->type = *p++;
        key->mode = *p++;
        strncpy(key->unit, p, UNIT_LEN);
        p += UNIT_LEN;
        appendTailList(&head, key);
    }
    dev->keyList_head = head;
    //register
    travelList(*getToDoListHead(), (manipulate_callback)registerTodo, NULL);
    travelList(*getSetListHead(), (manipulate_callback)registerSet, NULL);
    return dev;
err:
    //
    return NULL;
}

int dev_getData(const char *buf, uint32_t buflen, DevConfig_t *devConfig)
{
    if(buf == NULL || devConfig == NULL)
        return -1;
    struct safa_data data = {.buflen = buflen, .buf = (char *)buf};
    //keylist
    travelList(devConfig->keyList_head, (manipulate_callback)setKeyValue_move_safe, &data);
    return 0;
}

uint32_t dev_makeData(char *buf, _key_t *key)
{
    memcpy(buf, key->name, KEY_LEN);
    buf += KEY_LEN;
    return valueToBuf(key, (void **)&buf) + KEY_LEN;
}
