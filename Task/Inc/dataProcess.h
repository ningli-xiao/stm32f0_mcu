//
// Created by 咚咚 on 2022/2/10.
//

#ifndef MCU_DATAPROCESS_H
#define MCU_DATAPROCESS_H

#define SEND_TIME 30 //s

/* 接受报文类型 */
typedef enum {
    ChangeMsg = 0x00, //切换模式
    ControlSetMsg  = 0x01, // 控制参数设置
    AlarmSetMsg  = 0x02, // 报警阈值设置
    LightSetMsg  = 0x03, // 光照控制
    HeartMsg  = 0x04, // 心跳包

}Msg_LIST;

extern void dataProcessTask();
#endif //MCU_DATAPROCESS_H
