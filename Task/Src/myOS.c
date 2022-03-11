//
// Created by 咚咚 on 2022/1/26.
//
#include "myOS.h"
#include "mqtt.h"
#include "dataProcess.h"
#include "boardsComm.h"
#include "iwdg.h"
#if 1
/*
任务初始化
每一个结构体都代表一个任务
添加任务和删减任务都在这里完成
*/
struct TaskStruct TaskST[]={
        {0,100,0,mqttTask},
        {0,100,0,boardsCommTask},
//      {0,100,0,dataProcessTask},
        {0,10,0,feedTask},
};

/*
框架初始化
主要是初始化定时器和计算一些框架运行需要的数据
*/
uint8_t TaskCount=sizeof(TaskST)/sizeof(TaskST[0]);	//记录任务数量

//放中断中执行，用于任务计时
void OS_IT_RUN(){
    uint8_t i;
    for(i=0;i<TaskCount;i++){//遍历所有循环
        if(!TaskST[i].TaskStatus){//当任务不在挂起状态，执行以下步骤
            if(++TaskST[i].TaskTickNow >= TaskST[i].TaskTickMax){//计时，并判断是否到达定时时间
                //如果到了定时时间，则将任务挂起，并复位计时
                TaskST[i].TaskTickNow = 0;
                TaskST[i].TaskStatus = 1;
            }
        }
    }
}
//放在main函数中执行，自带死循环，用于执行挂起的任务
void PeachOSRun(){
    uint8_t j=0;
    while(1){
        if(TaskST[j].TaskStatus){		//判断一个任务是否被挂起
            TaskST[j].FC();				//执行该任务函数
            TaskST[j].TaskStatus=0;		//取消任务的挂起状态
        }
        if(++j>=TaskCount)j=0;		//相当于不断循环遍历所有的任务	
    }
}

#endif
