//
// Created by 咚咚 on 2022/1/26.
//

#ifndef STM32F0_MYOS_H
#define STM32F0_MYOS_H

#include "stm32f0xx_hal.h"

//框架运行所需的函数声明
void OS_IT_RUN(void);
void PeachOSRun(void);
void TIMCallBlack(void);

/*
用与存储一个任务运行的数据
TaskTickNow		用于计时
TaskTickMax		设置计时时间
TaskStatus		任务运行标志位
void (*FC)()	任务函数指针
*/
struct TaskStruct{
    uint16_t	TaskTickNow;
    uint16_t	TaskTickMax;
    uint8_t	TaskStatus;
    void (*FC)();
};

extern struct TaskStruct TaskST[];	//声明为结构体数据，那样多个任务时方便管理


//用于示例的任务函数声明

#endif //STM32F4_MYOS_H
