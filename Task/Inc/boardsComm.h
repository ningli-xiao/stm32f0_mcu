//
// Created by ßËßË on 2022/2/11.
//

#ifndef MCU_BOARDSCOMM_H
#define MCU_BOARDSCOMM_H
#include "stm32f0xx_hal.h"
extern uint8_t boardSendFlag;
extern uint8_t boardSendOkFlag ;
extern void boardsCommTask();
extern int SendtoBoard(char *message);
#endif //MCU_BOARDSCOMM_H
