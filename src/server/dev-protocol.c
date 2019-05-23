//dev-protocol.c

#include "dev-protocol.h"
#include "../log.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>


#define  MASKEY_LEN  4

DevConfig_t *dev_handShake(const char *buf, uint32_t buflen)
{
    if(buf == NULL || buflen < 3 + ID_LEN)  goto err;
    if(buf[0] != DEV_MAGIC[0] || buf[1] != DEV_MAGIC[1] ||
       buf[2] != DEV_MAGIC[2])
        goto err;
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
    DevConfig_t *dev = seachOneByRequired(devlist_head, 
                        (required_callback)isDevId, &id);
    if(!dev)
    {
        dev = initDevConfig();
        if(dev == NULL) return NULL;
        appendTailList(&devlist_head, dev);
        strncpy(dev->id, id, ID_LEN);
        //name
        strcpy(dev->name, "new device");
    }
    else
    {
        if (dev->keyList_head)   deleteList(&dev->keyList_head, NULL);
        if (dev->ep_event)   
        {
            loge("<dev_handShake>Device{%s} haven connected to this server.\n", dev->id);
            return NULL;
        }
    }
    dev->isOnline = true;
    dev->type = type;
    //keyList
    uint8_t keyNum = *p++;
    if(buflen < 3 + ID_LEN + sizeof(dev_type_t) + 
       KEY_COUNT_BYTE + keyNum * (KEY_LEN + 1 + 1 + UNIT_LEN)) 
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
    travelList(todolist_head, (manipulate_callback)registerTodo, NULL);
    travelList(setlist_head, (manipulate_callback)registerSet, NULL);
    return dev;
err:
    //
    loge("<initDev>len parameter illegal.\n");
    return NULL;
}

int dev_getData(const char *buf, uint32_t buflen, DevConfig_t *devConfig)
{
    if(buf == NULL || devConfig == NULL)
        return -1;
    if(*buf != DEV_PRO_UPDATE) return -1;
    buf++;
    buflen--;
    struct safa_data data = {.buflen = buflen, .buf = (char *)buf};
    //keylist
    travelList(devConfig->keyList_head, 
               (manipulate_callback)setKeyValue_move_safe, &data);
    //if(data.buflen > 0) return -1;
    return 0;
}

uint32_t dev_makeData(char *buf, _key_t *key)
{
    *buf++ = DEV_PRO_SET;
    memcpy(buf, key->name, KEY_LEN);
    buf += KEY_LEN;
    return valueToBuf(key, (void **)&buf) + KEY_LEN + 1;
}
