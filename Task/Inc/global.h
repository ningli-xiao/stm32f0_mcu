//
// Created by 咚咚 on 2022/1/25.
//

#ifndef _GLOBAL_H
#define _GLOBAL_H
#include "stm32f0xx_hal.h"
#include "stdio.h"
#include "main.h"
#include "string.h"
#include "stdlib.h"
#include "usart.h"
//加入以下代码,支持printf函数,而不需要选择use MicroLIB

#define MSG_REC_LEN 128
#define MSG_SEND_LEN 128

#define BOARDS_REC_LEN 128
#define BOARDS_SEND_LEN 128

//打印调试封装,正式发布时DEBUG设置为取消打印
#define  DEBUG   1
#if DEBUG
#define DBG_PRINTF(fmt, args...)  \
do\
{\
    printf("Line:%d  Function:%s>> ",__LINE__, __FUNCTION__);\
    printf(fmt, ##args);\
}while(0)
#else
#define DBG_PRINTF(fmt, args...)\
do\
{\
}while(0)
#endif
typedef struct
{
    uint8_t boardSendTimer1s;//
}ProcessTask_timer;
/* 按键状态机的状态 */
typedef enum {
    MQTT_OFFLINE = 0x00, //离线
    MQTT_LOGIN  = 0x01, // 正在登录
    MQTT_ONLINE  = 0x02, // 在线
}MQTT_STATUS_LIST;

/* 控制模式的状态 */
typedef enum {
    MQTT_AUTO = 0x00, //自动
    MQTT_MANUAL  = 0x01, // 手动
}MQTT_MODE_LIST;

/* 状态机类 */
typedef struct
{
    MQTT_STATUS_LIST MQTT_STATUS;
    MQTT_MODE_LIST MQTT_MODE;
}MQTT_COMPONENTS;


/*******************以下为全局变量区域*********************/
extern uint8_t msgSendBuff[MSG_SEND_LEN]; //发送缓存区
extern uint8_t msgRecBuff[MSG_REC_LEN]; //接收缓存区
extern uint8_t boardsSendBuff[MSG_SEND_LEN]; //发送缓存区
extern uint8_t boardsRecBuff[MSG_REC_LEN]; //接收缓存区

extern MQTT_COMPONENTS MCU_STATUS;
extern ProcessTask_timer Task_timer;

extern uint8_t msgRxFlag;       //接收完成标记
extern uint32_t msgRxSize;
extern uint8_t boardsRxFlag;       //接收完成标记
extern uint32_t boardsRxSize;
extern uint8_t boardsDownFlag;
extern uint8_t io2DownFlag;
/*******************全局函数接口*********************/
extern char* Int2String(int num,char *str);//10进制
extern char* FindStrFroMem(char *buf, uint16_t buflen, char *str);
extern uint8_t CheckXorAndMod(uint8_t *data, uint32_t len);
#endif
