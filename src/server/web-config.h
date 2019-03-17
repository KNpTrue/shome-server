/**
 * web相关的配置文件
*/
#ifndef _WEB_CONFIG_H
#define _WEB_CONFIG_H

#define PASSWD_LEN 32

typedef struct WebConfig {
    //端口配置
    short web_port;
    short web6_port;
    short dev_port;
    short dev6_port;
    //登陆密码
    char passwd[PASSWD_LEN + 1];
} WebConfig_t;

WebConfig_t *getWebConfig();


#endif //_WEB_CONFIG_H