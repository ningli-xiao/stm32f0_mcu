//
// Created by 咚咚 on 2022/1/26.
//
#include "mqtt.h"
#include "global.h"
#include "boardsComm.h"

uint8_t MODULE_IMEI[16] = {"0"};
uint8_t MODULE_ICCID[21] = {"0"};
uint8_t mqttTopic[30] = {"0"};//根据IMEI和topic构建新主题
MQTTPacket_connectData mqttConnectData = MQTTPacket_connectData_initializer;

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
        DBG_PRINTF("UART_SendData ERROR\r\n");
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
        HAL_Delay(1);
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
    //pRet = FindStrFroMem((char *) msgRecBuff, msgRxSize, "AT+GSN");
    DBG_PRINTF("msgRecBuff=%s\r\n", msgRecBuff);
    memcpy(&MODULE_IMEI[0], &msgRecBuff[2], 15);//去除0D/0A
    DBG_PRINTF("LTE_GET_IMEI Success:%s\r\n", &MODULE_IMEI[0]);
    return 0;
}

//获取ICCID号
int GET_ICCID(void) {
    char *pRet = 0;
    uint8_t i = 0;
    if (SendATCommand("AT+QCCID\r\n", "OK", WAIT_TIME_IN) == 0) {
        DBG_PRINTF("LTE_GET_ICCID Fail\r\n");
        return -1;
    }
    pRet = FindStrFroMem((char *) msgRecBuff, msgRxSize, "+QCCID:");
    DBG_PRINTF("pRet=%s\r\n", pRet);
    memcpy(&MODULE_ICCID[0], pRet + 8, 20);
    DBG_PRINTF("LTE_GET_ICCID Success:%s\r\n", MODULE_ICCID);
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

    uint32_t timestamp = 0;

    uint16_t year_temp = 0;
    uint8_t month_temp = 0;
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
        month_temp = (pRet[10] - 0x30) * 10 + (pRet[11] - 0x30);
        day_temp = (pRet[13] - 0x30) * 10 + (pRet[14] - 0x30);

        hour_temp = (pRet[16] - 0x30) * 10 + (pRet[17] - 0x30);
        minute_temp = (pRet[19] - 0x30) * 10 + (pRet[20] - 0x30);
        sec_temp = (pRet[22] - 0x30) * 10 + (pRet[23] - 0x30);
        timestamp = time_stamp_Set(year_temp, month_temp, day_temp, hour_temp, minute_temp, sec_temp);
        return timestamp;
    }
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
        if (GET_IMEI() == 0) {
            return 0;
        }
        HAL_Delay(1000);
    }
    DBG_PRINTF(" LTE_GET_IMEI  Timeout \r\n");
    return -1;
}

int Wait_GET_ICCID_RDY(uint8_t time) {
    while (--time) {
        if (GET_ICCID() == 0) {
            return 0;
        }
        HAL_Delay(1000);
    }
    DBG_PRINTF(" LTE_GET_ICCID  Timeout \r\n");
    return -1;
}

/*
 * 函数名：MQTTParam_init
 * 功能：参数初始化
 * 输入：无
 * 返回：无
 */
int MQTTParam_init() {
    mqttConnectData.MQTTVersion = 4;
    mqttConnectData.clientID.cstring = CLIENT_ID;
    mqttConnectData.username.cstring = CLIENT_USER;
    mqttConnectData.password.cstring = CLIENT_PASS;
    mqttConnectData.address.cstring = BROKER_SITE;
    mqttConnectData.port = BROKER_PORT;
}


/*
 * 函数名：MQTTClient_init
 * 功能：开机初始化等
 * 输入：无
 * 返回：imei topic
 */
int MQTTClient_init() {
    if (SendATCommand("ATE0\r\n", "OK", WAIT_TIME_OUT) == 0) {
        DBG_PRINTF("MQTT_OPENING\r\n");
        ModuleOpen();
       if(Wait_LTE_RDY(5)!=0){
				 return -1;
			 }
    } else {
        DBG_PRINTF("MQTT was OPENED\r\n");
    }

    if (SendATCommand("AT+QMTCFG=\"version\",0,4\r\n", "OK", WAIT_TIME_IN) != 0) {
    }
    DBG_PRINTF("AT+QMTCFG:%s\r\n", msgRecBuff);

    Wait_GET_IMEI_RDY(3);
    Wait_GET_ICCID_RDY(3);
    Wait_Signal_RDY(3);

    strcpy(mqttTopic, PUBLISH_TOPIC);
    strcat(mqttTopic, &MODULE_IMEI[7]);//拼接后8位数据:8-15
    DBG_PRINTF("mqttTopic:%s\r\n", mqttTopic);
    return 0;
}

/*
 * 函数名：MQTTClient_connect
 * 功能：发送注册包
 * 输入：无
 * 返回：无
 */
int MQTTClient_connect(MQTTPacket_connectData mqtt) {
    char buf[128] = {0};
    char *pRet = 0;
    char portTemp[5] = {0};

    sprintf(portTemp, "%d", mqtt.port);
    strcpy(buf, "AT+QMTOPEN=0,\"");
    strcat(buf, mqtt.address.cstring);
    strcat(buf, "\",");
    strcat(buf, portTemp);
    strcat(buf, "\r\n");//模式
    DBG_PRINTF("%s\r\n", buf);

    if (SendATCommand(buf, "+QMTOPEN: 0,0", WAIT_TIME_OUT) == 0) {
        DBG_PRINTF("error:%s\r\n", msgRecBuff);
        return -1;
    }
    DBG_PRINTF("AT+QMTOPEN:%s\r\n", msgRecBuff);

    memset(buf, 0, 128);//必须先清空
    strcpy(buf, "AT+QMTCONN=0,\"");
    strcat(buf, mqtt.clientID.cstring);
    strcat(buf, "\",\"");
    strcat(buf, mqtt.username.cstring);
    strcat(buf, "\",\"");
    strcat(buf, mqtt.password.cstring);
    strcat(buf, "\"\r\n");//模式
    DBG_PRINTF("AT+QMTCONN BUF:%s\r\n", buf);

    if (SendATCommand(buf, "+QMTCONN: 0,0,0", WAIT_TIME_OUT) == 0) {
        DBG_PRINTF("error:%s\r\n", msgRecBuff);
        return -1;
    }
    DBG_PRINTF("AT+QMTCONN BUF:%s\r\n", msgRecBuff);
    HAL_Delay(200);

    return 0;//通过返回0
}

/*
 * 函数名：MQTTClient_connect
 * 功能：发送注册包
 * 输入：无
 * 返回：无
 */
int MQTTClient_disconnect() {
    if (SendATCommand("AT+QMTDISC=0\r\n", "OK", WAIT_TIME_OUT) == 0) {
        return -1;
    }
    HAL_Delay(200);
    if (SendATCommand("AT+QMTCLOSE=0\r\n", "OK", WAIT_TIME_OUT) == 0) {
        return -1;
    }
    return 0;
}


/*
 * 函数名：GetCellLoc
 * 功能：获取定位数据
 * 输入：无
 * 返回：无
 */
int GetCellLoc()
{
    if (SendATCommand("AT+QIDEACT=1\r\n", "OK",WAIT_TIME_OUT) == 0) {
        DBG_PRINTF("QIDEACT :%s\r\n", msgRecBuff);
        //return -1;
    }
    DBG_PRINTF("QIDEACT :%s\r\n", msgRecBuff);

    if (SendATCommand("AT+QICSGP=1,1,\"CMNET\","","",1\r\n", "OK",WAIT_TIME_OUT) == 0) {
        DBG_PRINTF("QICSGP :%s\r\n", msgRecBuff);
        return -1;
    }
    DBG_PRINTF("QICSGP :%s\r\n", msgRecBuff);

    if (SendATCommand("AT+QIACT=1\r\n", "OK",WAIT_TIME_OUT) == 0) {
        DBG_PRINTF("QIACT :%s\r\n", msgRecBuff);
        //return -1;
    }
    DBG_PRINTF("QIACT :%s\r\n", msgRecBuff);


    if (SendATCommand("AT+QLOCCFG=\"contextid\",1\r\n", "OK", WAIT_TIME_OUT) == 0) {
        DBG_PRINTF("contextid :%s\r\n", msgRecBuff);
        //return -1;
    }
    DBG_PRINTF("contextid :%s\r\n", msgRecBuff);

    if (SendATCommand("AT+QLOCCFG=\"timeout\",10\r\n", "OK", WAIT_TIME_OUT) == 0) {
        DBG_PRINTF("timeout :%s\r\n", msgRecBuff);
        //return -1;
    }
    DBG_PRINTF("timeout :%s\r\n", msgRecBuff);

    if (SendATCommand("AT+QLOCCFG=\"tocken\",\"QM162r747j189N3Q\"\r\n", "OK", WAIT_TIME_OUT) == 0) {
        DBG_PRINTF("tocken :%s\r\n", msgRecBuff);
       // return -1;
    }
    DBG_PRINTF("tocken :%s\r\n", msgRecBuff);

//    if (SendATCommand("AT+QLOCCFG=\"latorder\",1\r\n", "OK", WAIT_TIME_OUT) == 0) {
//        DBG_PRINTF("latorder :%s\r\n", msgRecBuff);
//        return -1;
//    }
//    DBG_PRINTF("latorder :%s\r\n", msgRecBuff);

    if (SendATCommand("AT+QCELLLOC\r\n", "OK",WAIT_TIME_OUT) == 0) {
        DBG_PRINTF("QCELLLOC :%s\r\n", msgRecBuff);
        //return -1;
    }
    DBG_PRINTF("QCELLLOC :%s\r\n", msgRecBuff);

    DBG_PRINTF("loc :%s\r\n", msgRecBuff);
    return 0;
}

/*
 * 函数名：MQTTClient_pubMessage
 * 功能：发送注册包
 * 输入：无
 * 返回：无
 */
int MQTTClient_pubMessage(char *pubTopic, char *payload) {
    char buf[128] = {0};
    char lenTemp[4] = {0};
    sprintf(lenTemp, "%d", strlen(payload));


    strcpy(buf, "AT+QMTPUBEX=0,0,0,0,\"");
    strcat(buf, pubTopic);
    strcat(buf, "\",");
    strcat(buf, lenTemp);
    strcat(buf, "\r\n");//模式

    if (SendATCommand(buf, ">", WAIT_TIME_OUT) == 0) {
        DBG_PRINTF("pub error:%s\r\n", msgRecBuff);
        return -1;
    }

    if (SendATCommand(payload, "+QMTPUBEX: 0,0,0", WAIT_TIME_OUT) == 0) {
        DBG_PRINTF("payload error:%s\r\n", msgRecBuff);
        return -1;
    }
    return 0;
}

/*
 * 函数名：MQTTClient_subMessage
 * 功能：订阅消息
 * 输入：无
 * 返回：无
 */
int MQTTClient_subMessage(char *subTopic) {
    char buf[128] = {0};

    strcpy(buf, "AT+QMTSUB=0,1,\"");
    strcat(buf, subTopic);
    strcat(buf, "\",");
    strcat(buf, "2");
    strcat(buf, "\r\n");//模式
    DBG_PRINTF("%s\r\n", buf);

    if (SendATCommand(buf, "+QMTSUB: 0,1,0,2", WAIT_TIME_OUT) == 0) {
        DBG_PRINTF("sub error:%s\r\n", msgRecBuff);
        return -1;
    }
    return 0;
}

/*
 * 函数名：MQTTClient_checkMessage
 * 功能：检查收到的消息是否正确
 * 输入：无
 * 返回：无
 */
char *MQTTClient_checkMessage(char *message, char *topic) {
    //假设校验通过,//0X5A="Z";0X4e="N";
    uint8_t checkValue = 0;
    char checkValueStr[2] = {0};
    char *ptr = NULL;
    char *ptrN = NULL;
    char *ptrZ = NULL;
    ptr = FindStrFroMem(message, msgRxSize, topic);
    if (ptr == 0) {
        DBG_PRINTF("not found topic \r\n");
        return NULL;
    }
    //DBG_PRINTF("ptr befor%s\r\n",ptr);


    ptr = strstr(ptr, ",");
    ptr += 2;//字符串开始位置
    //DBG_PRINTF("ptr check%s\r\n",ptr);
    //查找分隔符位置
    ptrN = strstr(ptr, "N");
    ptrZ = strstr(ptr, "Z");

    if ((ptrZ - ptrN) == 2) {
        memcpy(checkValueStr, ptrN + 1, 1);
        checkValue = atoi(checkValueStr);
    } else if ((ptrZ - ptrN) == 3) {
        memcpy(checkValueStr, ptrN + 1, 2);
        checkValue = atoi(checkValueStr);
    } else {
        DBG_PRINTF("checkValue len error:%d\r\n", ptrZ - ptrN);
        return NULL;
    }

    if (CheckXorAndMod(ptr, (ptrN - ptr )) != checkValue) {
        DBG_PRINTF("%d\r\n", checkValue);
        return NULL;
    }
    *(++ptrZ) = 0;
    return ptr;
}

/*
 * 函数名：MQTTClient_tunsMessage
 * 功能：退订消息
 * 输入：无
 * 返回：无
 */
int MQTTClient_tunsMessage(char *subTopic) {
    char buf[128] = {0};

    strcpy(buf, "AT+QMTUNS=0,2,\"");
    strcat(buf, subTopic);
    strcat(buf, "\"");
    strcat(buf, "\r\n");//模式
    DBG_PRINTF("%s\r\n", buf);

    if (SendATCommand(buf, "OK", WAIT_TIME_OUT) == 0) {
        DBG_PRINTF("tuns error:%s\r\n", msgRecBuff);
        return -1;
    }
    return 0;
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
            if(MQTTClient_init()!=0)
            {
                return ;
            }
            MQTTParam_init();
            HAL_Delay(500);
            MCU_STATUS.MQTT_STATUS = MQTT_LOGIN;
            break;

        case MQTT_LOGIN:
            if (MQTTClient_disconnect() == 0)//保护作用，防止重启error
            {
                DBG_PRINTF("disconnect ok\r\n");
            }
            MQTTClient_connect(mqttConnectData);
            net_time = LTE_Get_Real_Time();

            MQTTClient_subMessage(SUBSCRIBE_TOPIC);
            HAL_Delay(500);
            MCU_STATUS.MQTT_STATUS = MQTT_ONLINE;
            break;

        case MQTT_ONLINE:
            if (boardSendFlag == 1) {
                boardSendFlag = 0;

                if (MQTTClient_pubMessage(mqttTopic, msgSendBuff) == 0) {
                    boardSendOkFlag = 1;
                }
            }

            if (msgRxFlag == 1) {
                msgRxFlag = 0;
                char *ptr = NULL;
                ptr = MQTTClient_checkMessage(msgRecBuff, SUBSCRIBE_TOPIC);
                if (ptr != NULL) {
                    memset(boardsSendBuff, 0, BOARDS_SEND_LEN);
                    strcpy(boardsSendBuff, ptr);
                    boardsDownFlag = 1;
                }
            }

            if (Task_timer.publishTimer1s == 0) {
                Task_timer.publishTimer1s = PUBLISH_INTERVAL;
                 GetCellLoc();
//                if (MQTTClient_pubMessage(mqttTopic, "LOC:") == 0) {
//                    boardSendOkFlag = 1;
//                }
            }

            break;
        default:
            break;
    }
}

