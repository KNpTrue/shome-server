/**
 * dev-type.h
 * 设备的类型
*/

#ifndef _DEV_TYPE_H
#define _DEV_TYPE_H

enum dev_type {
    SM_SWITCH,  //开关
    SM_CAMERA,   //视频设备
    SM_LIGHT,   //灯
    SM_TEMPL,   //温湿度传感器
    SM_AIR_CON, //空调
};

#endif //_DEV_TYPE_H