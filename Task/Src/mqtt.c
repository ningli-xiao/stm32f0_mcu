//
// Created by ���� on 2022/1/26.
//
#include "mqtt.h"
#include "global.h"
#include "boardsComm.h"

uint8_t MODULE_IMEI[16] = {"0"};
uint8_t MODULE_ICCID[21] = {"0"};
uint8_t mqttTopic[30] = {"0"};//����IMEI��topic����������
MQTTPacket_connectData mqttConnectData = MQTTPacket_connectData_initializer;

/*
 * ��������SoftReset
 * ���ܣ������λ�����ܵ�OTAָ���ִ��
 * ���룺��
 * ���أ���
 */
static void SoftReset(void) {
    //__set_FAULTMASK(1);
    HAL_NVIC_SystemReset();
}

/*
 * ��������ModuleClose
 * ���ܣ�4Gģ��ػ�
 * ���룺��
 * ���أ���
 */
void ModuleClose(void) {
    HAL_GPIO_WritePin(ONOFF_EC200_GPIO_Port, ONOFF_EC200_Pin, GPIO_PIN_SET);
    HAL_Delay(1500);
    HAL_GPIO_WritePin(ONOFF_EC200_GPIO_Port, ONOFF_EC200_Pin, GPIO_PIN_RESET);
}

/*
 * ��������ModuleOpen
 * ���ܣ�4Gģ�鿪��
 * ���룺��
 * ���أ���
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
 * ��������SendATCommand
 * ���ܣ�����ATָ��
 * ���룺��
 * ���أ���
 */
char *SendATCommand(char *pCommand, char *pEcho, uint32_t outTime) {
    char *pRet = NULL;
    int i = 0;
    if (NULL != pCommand) {
        memset(msgRecBuff, 0, MSG_REC_LEN);//���������
        UART_SendData((uint8_t *) pCommand);
    }
    while (--outTime)//�ȴ�ָ��ظ�
    {
        if (1 == msgRxFlag) {
            msgRxFlag = 0;
            pRet = FindStrFroMem(msgRecBuff, msgRxSize, pEcho);
            if (pRet != 0) { return pRet; }//����ȷ�ظ�ʱ���غ���
        }
        HAL_Delay(1);
    }
    return pRet;
}

//��ȡIMEI��
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
    DBG_PRINTF("msgRecBuff=%s\r\n", msgRecBuff);
    memcpy(&MODULE_IMEI[0], pRet + 9, 15);
    DBG_PRINTF("LTE_GET_IMEI Success:%s\r\n", &MODULE_IMEI[0]);
    return 0;
}

//��ȡICCID��
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

//�ж��ź�����
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
 * ��������MQTTParam_init
 * ���ܣ�������ʼ��
 * ���룺��
 * ���أ���
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
 * ��������MQTTClient_init
 * ���ܣ�������ʼ����
 * ���룺��
 * ���أ�imei topic
 */
int MQTTClient_init() {
    if (SendATCommand("ATE0\r\n", "OK", WAIT_TIME_OUT) == 0) {
        DBG_PRINTF("MQTT_OPENING\r\n");
        ModuleOpen();
        Wait_LTE_RDY(5);//�ȴ�����
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
    strcat(mqttTopic, &MODULE_IMEI[8]);//ƴ�Ӻ�8λ����
    DBG_PRINTF("mqttTopic:%s\r\n", mqttTopic);
}

/*
 * ��������MQTTClient_connect
 * ���ܣ�����ע���
 * ���룺��
 * ���أ���
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
    strcat(buf, "\r\n");//ģʽ
    DBG_PRINTF("%s\r\n", buf);

    if (SendATCommand(buf, "+QMTOPEN: 0,0", WAIT_TIME_OUT) == 0) {
        DBG_PRINTF("error:%s\r\n", msgRecBuff);
        return -1;
    }
    DBG_PRINTF("AT+QMTOPEN:%s\r\n", msgRecBuff);

    memset(buf, 0, 128);//���������
    strcpy(buf, "AT+QMTCONN=0,\"");
    strcat(buf, mqtt.clientID.cstring);
    strcat(buf, "\",\"");
    strcat(buf, mqtt.username.cstring);
    strcat(buf, "\",\"");
    strcat(buf, mqtt.password.cstring);
    strcat(buf, "\"\r\n");//ģʽ
    DBG_PRINTF("AT+QMTCONN BUF:%s\r\n", buf);

    if (SendATCommand(buf, "+QMTCONN: 0,0,0", WAIT_TIME_OUT) == 0) {
        DBG_PRINTF("error:%s\r\n", msgRecBuff);
        return -1;
    }
    DBG_PRINTF("AT+QMTCONN BUF:%s\r\n", msgRecBuff);
    HAL_Delay(200);

    return 0;//ͨ������0
}

/*
 * ��������MQTTClient_connect
 * ���ܣ�����ע���
 * ���룺��
 * ���أ���
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
 * ��������MQTTClient_pubMessage
 * ���ܣ�����ע���
 * ���룺��
 * ���أ���
 */
int MQTTClient_pubMessage(char *pubTopic, char *payload) {
    char buf[128] = {0};
    char lenTemp[4]={0};
    sprintf(lenTemp,"%d",strlen(payload));


    strcpy(buf, "AT+QMTPUBEX=0,0,0,0,\"");
    strcat(buf, pubTopic);
    strcat(buf, "\",");
    strcat(buf, lenTemp);
    strcat(buf, "\r\n");//ģʽ

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
 * ��������MQTTClient_subMessage
 * ���ܣ�������Ϣ
 * ���룺��
 * ���أ���
 */
int MQTTClient_subMessage(char *subTopic) {
    char buf[128] = {0};

    strcpy(buf, "AT+QMTSUB=0,1,\"");
    strcat(buf, subTopic);
    strcat(buf, "\",");
    strcat(buf, "2");
    strcat(buf, "\r\n");//ģʽ
    DBG_PRINTF("%s\r\n", buf);

    if (SendATCommand(buf, "OK", WAIT_TIME_OUT) == 0) {
        DBG_PRINTF("sub error:%s\r\n", msgRecBuff);
        return -1;
    }
    return 0;
}

/*
 * ��������MQTTClient_tunsMessage
 * ���ܣ��˶���Ϣ
 * ���룺��
 * ���أ���
 */
int MQTTClient_tunsMessage(char *subTopic) {
    char buf[128] = {0};

    strcpy(buf, "AT+QMTUNS=0,2,\"");
    strcat(buf, subTopic);
    strcat(buf, "\"");
    strcat(buf, "\r\n");//ģʽ
    DBG_PRINTF("%s\r\n", buf);

    if (SendATCommand(buf, "OK", WAIT_TIME_OUT) == 0) {
        DBG_PRINTF("tuns error:%s\r\n", msgRecBuff);
        return -1;
    }
    return 0;
}

/*
 * ��������mqttTask
 * ���ܣ�mqtt����ִ�У�״̬����ʽ
 * ���룺��
 * ���أ���
 */
void mqttTask() {
    static uint8_t restart_count = 0;
    switch (MCU_STATUS.MQTT_STATUS) {
        case MQTT_OFFLINE:
            DBG_PRINTF("MQTT_OFFLINE\r\n");
            MQTTClient_init();
            MQTTParam_init();
            MCU_STATUS.MQTT_STATUS = MQTT_LOGIN;
            break;

        case MQTT_LOGIN:
            if (MQTTClient_disconnect() == 0)//�������ã���ֹ����error
            {
                DBG_PRINTF("disconnect ok\r\n");
            }
            MQTTClient_connect(mqttConnectData);
            net_time = LTE_Get_Real_Time();
            DBG_PRINTF("TIME:%lu\r\n", net_time);
            MCU_STATUS.MQTT_STATUS = MQTT_ONLINE;
            break;

        case MQTT_ONLINE:
            if (boardSendFlag==1){
                boardSendFlag=0;
                if(MQTTClient_pubMessage(mqttTopic,msgSendBuff)==0)
                {
                    boardSendOkFlag=1;
                }
            }
            if(msgRxFlag==1)
            {
                msgRxFlag=0;
                //����У��ͨ��
                memset(boardsSendBuff,0,BOARDS_SEND_LEN);
                memcpy(boardsSendBuff,msgRecBuff,msgRxSize);
                boardsDownFlag=1;
            }
            if (Task_timer.publishTimer1s == 0) {
                Task_timer.publishTimer1s = PUBLISH_INTERVAL;
                if(MQTTClient_pubMessage(mqttTopic,"LOC:")==0)
                {
                    boardSendOkFlag=1;
                }
            }

            break;
        default:


            break;
    }
}

