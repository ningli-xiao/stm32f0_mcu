//
// Created by 咚咚 on 2022/2/10.
//

#include "dataProcess.h"
#include "global.h"

/*
 * 函数名：SendHeartPacket
 * 功能：发送心跳包
 * 输入：无
 * 返回：无
 */
static int SendHeartPacket(void) {

}

/*
 * 函数名：RecDataPacket
 * 功能：处理接受到的数据
 * 输入：无
 * 返回：无
 */
static int RecDataPacket(uint8_t flag) {
    uint8_t msgType = 0;
    if (flag == 1) {
        //处理数据，首先判断报文类型
        switch (msgType) {
            case ChangeMsg:

                break;

            case ControlSetMsg:

                break;
            case AlarmSetMsg:

                break;
            case LightSetMsg:

                break;
            case HeartMsg:

                break;

        }

    } else {
        return 0;
    }
}

/*
 * 函数名：dataProcessTask
 * 功能：数据收发处理，定时发送心跳包:根据全局计时器，
 *      任务预留，如果以后把控制板功能加入启动此任务，目前功能较少，写入到mqttTask
 * 输入：无
 * 返回：无
 */
void dataProcessTask() {
    static uint16_t RecCount = 0;//接受计时
    switch (MCU_STATUS.MQTT_STATUS) {
        case MQTT_OFFLINE:
            DBG_PRINTF("MQTT_OFF DoNothing\r\n");
            break;

        case MQTT_LOGIN:
            DBG_PRINTF("MQTT_LOGIN DoNothing\r\n");

            break;

        case MQTT_ONLINE:
            DBG_PRINTF("MQTT_ONLINE\r\n");
            RecCount++;

            //判断是否串口收到数据,处理信息
            if (RecDataPacket(msgRxFlag) != 0) {
                RecCount = 0;
            }


            break;
        default:
            DBG_PRINTF("not run well\r\n");

            break;
    }
}