//
// Created by 咚咚 on 2022/1/26.
//
#include "mqtt.h"
#include "global.h"


uint8_t MODULE_IMEI[20] = {"0"};
uint8_t MODULE_ICCID[25] = {"0"};
uint8_t mqttTopic[20] = {"0"};//根据IMEI和topic构建新主题
uint32_t net_time = 0;

/*
 * 函数名：SoftReset
 * 功能：软件复位，接受到OTA指令后执行
 * 输入：无
 * 返回：无
 */
static void SoftReset(void) {
    //__set_FAULTMASK(1);
    HAL_NVIC_SystemReset();
}

/*
 * 函数名：ModuleClose
 * 功能：4G模组关机
 * 输入：无
 * 返回：无
 */
void ModuleClose(void) {
    HAL_GPIO_WritePin(ONOFF_EC200_GPIO_Port, ONOFF_EC200_Pin, GPIO_PIN_SET);
    HAL_Delay(1500);
    HAL_GPIO_WritePin(ONOFF_EC200_GPIO_Port, ONOFF_EC200_Pin, GPIO_PIN_RESET);
}

/*
 * 函数名：ModuleOpen
 * 功能：4G模组开机
 * 输入：无
 * 返回：无
 */
static void ModuleOpen(void) {
    HAL_GPIO_WritePin(ONOFF_EC200_GPIO_Port, ONOFF_EC200_Pin, GPIO_PIN_SET);
    HAL_Delay(1500);
    HAL_GPIO_WritePin(ONOFF_EC200_GPIO_Port, ONOFF_EC200_Pin, GPIO_PIN_RESET);
}

void UART_SendData(char *pdatabuf) {
    uint16_t sizeTemp = 0;
    sizeTemp = strlen(pdatabuf);
    if (HAL_UART_Transmit(&huart1, pdatabuf, sizeTemp, 1000) != HAL_OK) {
        DBG_PRINTF("UART_SendData ERROR");
    }
}

/*
 * 函数名：SendATCommand
 * 功能：发送AT指令
 * 输入：无
 * 返回：无
 */
char *SendATCommand(char *pCommand, char *pEcho, uint32_t outTime) {
    char *pRet = NULL;
    int i = 0;
    if (NULL != pCommand) {
        memset(msgRecBuff, 0, MSG_REC_LEN);//必须先清空
        UART_SendData((uint8_t *) pCommand);
    }
    while (--outTime)//等待指令回复
    {
        if (1 == msgRxFlag) {
            msgRxFlag = 0;
            pRet = FindStrFroMem(msgRecBuff, msgRxSize, pEcho);
            if (pRet != 0) { return pRet; }//有正确回复时返回函数
        }
        HAL_Delay(10);
    }
    return pRet;
}

//获取IMEI号
int GET_IMEI(void) {
    char *pRet = NULL;
    uint8_t i = 0;
    if (SendATCommand("AT+GSN\r\n", "OK", WAIT_TIME_IN) == 0) {
        DBG_PRINTF("LTE_GET_Imei Fail\r\n");
        return -1;
    }
    HAL_Delay(100);
    pRet = FindStrFroMem((char *) msgRecBuff, msgRxSize, "AT+GSN");
    DBG_PRINTF("pRet=%s\r\n", pRet);
    memcpy(&MODULE_IMEI[0], pRet + 9, 15);
    DBG_PRINTF("LTE_GET_IMEI Success:%s\r\n", &MODULE_IMEI[0]);
    return 0;
}

//获取ICCID号
int GET_ICCID(void) {
    char *pRet = 0;
    uint8_t i = 0;
    if (SendATCommand("AT+CCID\r\n", "OK", WAIT_TIME_IN) == 0) {
        DBG_PRINTF("LTE_GET_ICCID Fail\r\n");
        return -1;
    }
    pRet = FindStrFroMem((char *) msgRecBuff, msgRxSize, "+CCID:");
    pRet += 7;
    DBG_PRINTF("pRet=%s\r\n", pRet);
    for (i = 0; i < 10; i++) {
        MODULE_ICCID[i] = ((pRet[i * 2] - 0x30) << 4) | (pRet[i * 2 + 1] - 0x30);
    }
    DBG_PRINTF("LTE_GET_ICCID Success\r\n");
    return 0;
}

//判断信号质量
int GET_Signal_Quality(void) {
    char *pRet = 0;
    pRet = SendATCommand("AT+CSQ\r\n", "OK", WAIT_TIME_IN);
    if (pRet == 0) {
        return -1;
    }
    printf("signal_value:%s\r\n", msgRecBuff);
    if (strstr((char *) pRet, "99,99") != 0) {
        return -1;
    }
    return 0;
}

int LTE_Get_Real_Time(void) {
    uint16_t year_temp = 0;
    uint8_t momth_temp = 0;
    uint8_t day_temp = 0;

    uint8_t hour_temp = 0;
    uint8_t minute_temp = 0;
    uint8_t sec_temp = 0;

    char *pRet = NULL;
    uint32_t len = 0;
    pRet = SendATCommand("AT+CCLK?\r\n", "+CCLK:", WAIT_TIME_OUT);
    if (pRet != 0) {
        pRet += 1;
        year_temp = (pRet[7] - 0x30) * 10 + (pRet[8] - 0x30) + 2000;
        momth_temp = (pRet[10] - 0x30) * 10 + (pRet[11] - 0x30);
        day_temp = (pRet[13] - 0x30) * 10 + (pRet[14] - 0x30);

        hour_temp = (pRet[16] - 0x30) * 10 + (pRet[17] - 0x30);
        minute_temp = (pRet[19] - 0x30) * 10 + (pRet[20] - 0x30);
        sec_temp = (pRet[22] - 0x30) * 10 + (pRet[23] - 0x30);
        net_time = time_stamp_Set(year_temp, momth_temp, day_temp, hour_temp, minute_temp, sec_temp);
        DBG_PRINTF("TIME:%lu\r\n", net_time);
        return 0;
    }
    HAL_Delay(100);
    return -1;
}

int Wait_LTE_RDY(uint8_t time) {
    while (--time) {
        if (SendATCommand(NULL, "RDY", WAIT_TIME_IN) == 0) {
            printf(" No  RDY \r\n");
        } else {
            printf(" RDY Come\r\n");
            return 0;
        }
        HAL_Delay(1000);
    }
    printf(" Wait_LTE_RDY Timeout:%s\r\n", msgRecBuff);
    return -1;
}

int Wait_Signal_RDY(uint8_t time) {
    while (--time) {
        if (GET_Signal_Quality() != 0) {
            DBG_PRINTF(" No  Signal \r\n");
        } else {
            DBG_PRINTF(" Signal OK\r\n");
            return 0;
        }
        HAL_Delay(500);
    }
    DBG_PRINTF(" Wait_Signal_RDY Timeout\r\n");
    return -1;
}

int Wait_GET_IMEI_RDY(uint8_t time) {
    while (--time) {
        if (GET_IMEI() != 0) {
            DBG_PRINTF(" LTE_GET_IMEI  Faile \r\n");
        } else {
            DBG_PRINTF(" LTE_GET_IMEI  Success \r\n");
            return 0;
        }
        HAL_Delay(1000);
    }
    DBG_PRINTF(" LTE_GET_IMEI  Timeout \r\n");
    return -1;
}

int Wait_GET_ICCID_RDY(uint8_t time) {
    while (--time) {
        if (GET_ICCID() != 0) {
            DBG_PRINTF(" LTE_GET_ICCID  Faile \r\n");
        } else {
            DBG_PRINTF(" LTE_GET_ICCID  Success \r\n");
            return 0;
        }
        HAL_Delay(1000);
    }
    DBG_PRINTF(" LTE_GET_ICCID  Timeout \r\n");
    return -1;
}

/*
 * 函数名：MQTTClient_init
 * 功能：开机初始化等
 * 输入：无
 * 返回：imei topic
 */
int MQTTClient_init() {
    if (SendATCommand("ati\r\n", "OK", WAIT_TIME_IN) == 0) {
        DBG_PRINTF("MQTT_OPEN:%s\r\n", msgRecBuff);
        ModuleOpen();
        Wait_LTE_RDY(5);//等待开机
    } else {
        DBG_PRINTF("MQTT was OPENED\r\n");
    }
    if (SendATCommand("ATE0\r\n", "OK", WAIT_TIME_IN) != 0) {
    }
		
    if (SendATCommand("AT+QMTCFG=\"version\",0,4\r\n", "OK", WAIT_TIME_IN) != 0) {
    }
    DBG_PRINTF("AT+QMTCFG:%s\r\n", msgRecBuff);
    Wait_GET_IMEI_RDY(3);
    Wait_GET_ICCID_RDY(3);
    Wait_Signal_RDY(3);
    strcpy(mqttTopic, PUBLISH_TOPIC);
    strcat(mqttTopic, MODULE_IMEI);
    DBG_PRINTF("mqttTopic:%s\r\n", mqttTopic);
}

/*
 * 函数名：MQTTClient_connect
 * 功能：发送注册包
 * 输入：无
 * 返回：无
 */
int MQTTClient_connect() {

}

/*
 * 函数名：MQTTClient_connect
 * 功能：发送注册包
 * 输入：无
 * 返回：无
 */
int MQTTClient_disconnect(int client, char *conn_opts) {

}

/*
 * 函数名：MQTTClient_pubMessage
 * 功能：发送注册包
 * 输入：无
 * 返回：无
 */
int MQTTClient_pubMessage() {

}

/*
 * 函数名：MQTTClient_subMessage
 * 功能：发送注册包
 * 输入：无
 * 返回：无
 */
int MQTTClient_subMessage() {

}

/*
 * 函数名：mqttTask
 * 功能：mqtt任务执行，状态机方式
 * 输入：无
 * 返回：无
 */
void mqttTask() {
    static uint8_t restart_count = 0;
    switch (MCU_STATUS.MQTT_STATUS) {
        case MQTT_OFFLINE:
            DBG_PRINTF("MQTT_OFFLINE\r\n");

            MQTTClient_init();

#if 1

            if (SendATCommand("AT+CGPADDR=1\r\n", "OK", WAIT_TIME_OUT) != 0) {
            }
            DBG_PRINTF("run 0:%s\r\n", msgRecBuff);

            if (SendATCommand("AT+QMTCFG=\"aliauth\",0,\"guj8fzw4Cqm\",\"ning\",\"08b9d73fa2df4df8ea9024a8d1826a0e\"\r\n", "OK", WAIT_TIME_OUT) != 0) {
            }
            DBG_PRINTF("run 0:%s\r\n", msgRecBuff);

            if (SendATCommand("AT+CGPADDR=1\r\n", "OK", WAIT_TIME_OUT) != 0) {
            }
            DBG_PRINTF("run 0:%s\r\n", msgRecBuff);
            if (SendATCommand("AT+QMTOPEN=0,\"iot-06z00ajwgt8j2qm.mqtt.iothub.aliyuncs.com\",1883\r\n", "+QMTOPEN:0,0", WAIT_TIME_OUT) != 0) {
            }
            if (SendATCommand("AT+QMTCONN=0,\"ning\"\r\n", "+QMTCONN:0,0,0", WAIT_TIME_OUT) != 0){

            }
            DBG_PRINTF("AT+QMTCONN:%s\r\n", msgRecBuff);
#endif
            MCU_STATUS.MQTT_STATUS = MQTT_LOGIN;
            break;

        case MQTT_LOGIN:


            if (SendATCommand("AT+QMTSUB=0,1,\"topic/example\",2\r\n", "OK", WAIT_TIME_OUT) != 0) {

            }
            DBG_PRINTF("run 0:%s\r\n", msgRecBuff);

            if (SendATCommand("AT+QMTPUBEX=0,0,0,0,\"topic/pub\",30\r\n", ">", WAIT_TIME_OUT) != 0) {

            }
            DBG_PRINTF("run 0:%s\r\n", msgRecBuff);

            if (SendATCommand("this is test data\r\n", "OK", WAIT_TIME_OUT) != 0) {

            }
            DBG_PRINTF("run 0:%s\r\n", msgRecBuff);
//            if (SendATCommand("AT+QMTDISC=0\r\n", "OK", WAIT_TIME_OUT) != 0) {
//                DBG_PRINTF("run 0:%s\r\n", msgRecBuff);
//                DBG_PRINTF("MQTT_LOGIN\r\n");
//                
//            }
            MCU_STATUS.MQTT_STATUS = MQTT_ONLINE;

            break;

        case MQTT_ONLINE:


            break;
        default:


            break;
    }
}

