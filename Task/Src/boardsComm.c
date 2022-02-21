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
    char timeTemp[10]={0};
    switch (MCU_STATUS.MQTT_STATUS) {
        case MQTT_ONLINE:
            //上行中继
            if (boardsRxFlag == 1) {
                boardsRxFlag = 0;
                if (FindStrFroMem(boardsRecBuff, boardsRxSize, "RQ_TIME") == 0) {
                    if(boardSendFlag == 0 && boardsRxSize>0) {
                        memset(msgSendBuff, 0, MSG_SEND_LEN);
                        memcpy(msgSendBuff, boardsRecBuff, boardsRxSize);
                        boardSendFlag = 1;
                    }
                } else {
                    net_time=LTE_Get_Real_Time();
                    sprintf(timeTemp,"%lu",net_time);
                    memset(boardsSendBuff, 0, BOARDS_SEND_LEN);
                    strcpy(boardsSendBuff,"TIME:");
                    strcat(boardsSendBuff,timeTemp);
                    HAL_UART_Transmit(&huart2, boardsSendBuff, strlen(boardsSendBuff), 1000);//回复时间
                }
            }

            //下行中继
            if (boardsDownFlag == 1) {
                boardsDownFlag=0;
                HAL_UART_Transmit(&huart2, boardsSendBuff, strlen(boardsSendBuff), 1000);
            }

            if (boardSendOkFlag == 1) {
                boardSendOkFlag = 0;
                SendtoBoard("ALIVE88N");
            }
            break;

        default:
            if (Task_timer.boardSendTimer1s == 0) {
                Task_timer.boardSendTimer1s = 10;
                SendtoBoard("ALIVE99N");
            }
            break;

    }
}