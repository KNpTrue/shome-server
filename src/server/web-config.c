//web-config.c
#include "web-config.h"

static WebConfig_t webConfig = {
    .web_port = 8001,
    .web6_port = 8002,
    .dev_port = 8003,
    .dev6_port = 8004,
    .passwd = {0}
};

WebConfig_t *getWebConfig()
{
    return &webConfig;
}