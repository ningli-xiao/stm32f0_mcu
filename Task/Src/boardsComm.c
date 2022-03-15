//
// Created by 咚咚 on 2022/2/11.
//

#include "boardsComm.h"
#include "global.h"
#include "mqtt.h"

uint8_t boardSendFlag = 0;
uint8_t boardSendOkFlag = 0;

/*
 * 函数名：SendtoBoard
 * 功能：发送消息
 * 输入：无
 * 返回：无
 */
int SendtoBoard(char *message) {
    char buf[128] = {0};
    char checkTemp[2] = {0};
    uint8_t checkValue = 0;

    checkValue = CheckXorAndMod(message, 7);
    sprintf(checkTemp, "%d", checkValue);
    strcpy(buf, message);//*Z
    strcat(buf, checkTemp);
    strcat(buf, "Z");//*Z

    if (HAL_UART_Transmit(&huart2, buf, strlen(buf), 1000) != HAL_OK) {
        DBG_PRINTF("mcuSendBuff ERROR");
    }
    return 0;
}


/*
 * 函数名：boardsCommTask
 * 功能：负责和控制板通信业务
 * 输入：无
 * 返回：无
 */
void boardsCommTask() {
    uint8_t bufLen = 0;
    char timeTemp[10] = {0};

    if (rstDownFlag == 1) {
        rstDownFlag = 0;
        uint8_t rstValue = 0;
        uint8_t i = 0;
        for (i = 0; i < 8; i++) {
            HAL_Delay(2);
            rstValue += HAL_GPIO_ReadPin(RST_EC200_GPIO_Port, RST_EC200_Pin);
        }
        if (rstValue < 2) {
            MCU_STATUS.MQTT_STATUS = MQTT_ALL_RESTART;
        }
    }

    switch (MCU_STATUS.MQTT_STATUS) {
        case MQTT_ONLINE:
            //上行中继
            if (boardsGetTimeFlag == 1) {
                char checkTemp[3]={0};
                boardsGetTimeFlag = 0;

                net_time = LTE_Get_Real_Time();
                if(net_time>0){
                    net_time-=28800;//转换为北京时间,加8h
                }
                sprintf(timeTemp, "%lu", net_time);
                memset(boardsSendBuff, 0, BOARDS_SEND_LEN);
                strcpy(boardsSendBuff, "TIME:");
                strcat(boardsSendBuff, timeTemp);
                strcat(boardsSendBuff, "N");

                sprintf(checkTemp, "%d",  CheckXorAndMod(boardsSendBuff, strlen(boardsSendBuff) - 1));
                strcat(boardsSendBuff, checkTemp);
                strcat(boardsSendBuff, "Z");//模式

                HAL_UART_Transmit(&huart2, boardsSendBuff, strlen(boardsSendBuff), 1000);//回复时间
            }

            //下行中继
            if (boardsDownFlag == 1) {
                boardsDownFlag = 0;
                HAL_UART_Transmit(&huart2, boardsSendBuff, strlen(boardsSendBuff), 1000);
            }

            if (boardSendOkFlag == 1) {
                boardSendOkFlag = 0;
                SendtoBoard("ALIVE88N");
            }
            break;

        default:

            break;

    }
}