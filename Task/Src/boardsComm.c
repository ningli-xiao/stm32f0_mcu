//
// Created by ���� on 2022/2/11.
//

#include "boardsComm.h"
#include "global.h"

uint8_t boardSendFlag=0;
uint8_t boardSendOkFlag=0;
/*
 * ��������boardsCommTask
 * ���ܣ�����Ϳ��ư�ͨ��ҵ��
 * ���룺��
 * ���أ���
 */
void boardsCommTask() {
    uint8_t bufLen = 0;
    uint8_t checkValue = 0;
    switch (MCU_STATUS.MQTT_STATUS) {
        case MQTT_ONLINE:
            //�����м�
            if (boardSendFlag == 0) {
                if (boardsRxFlag == 1) {
                    boardsRxFlag = 0;
                    if (0) {//RQ_TIME
                        memset(msgSendBuff, 0, MSG_SEND_LEN);
                        memcpy(msgSendBuff, boardsRecBuff, boardsRxSize);
                        boardSendFlag = 1;
                    } else {
                        HAL_UART_Transmit(&huart1, boardsSendBuff, bufLen, 1000);
                    }
                }
            }

            //�����м�
            if(boardsDownFlag==1){
               HAL_UART_Transmit(&huart1, boardsSendBuff, bufLen, 1000) ;
            }

            if(boardSendOkFlag==1){
                boardSendOkFlag=0;
                memset(boardsSendBuff,0,BOARDS_SEND_LEN);
                strcpy(boardsSendBuff, "ALIVE88N80Z");
                HAL_UART_Transmit(&huart1, boardsSendBuff, bufLen, 1000);
            }

            DBG_PRINTF("MQTT_ONLINE board\r\n");
            break;

        default:
            DBG_PRINTF("MQTT_OFF board\r\n");
            memset(msgSendBuff,0,MSG_SEND_LEN);
            strcpy(msgSendBuff, "ALIVE99N*Z");
            checkValue = CheckXorAndMod(boardsSendBuff, 7);
            boardsSendBuff[8] = checkValue;
            bufLen = strlen(boardsSendBuff);
            if (Task_timer.boardSendTimer1s == 0) {
                Task_timer.boardSendTimer1s = 10;
                if (HAL_UART_Transmit(&huart1, boardsSendBuff, bufLen, 1000) != HAL_OK) {
                    DBG_PRINTF("mcuSendBuff ERROR");
                }
            }
            break;

    }
}