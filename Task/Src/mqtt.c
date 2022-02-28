//
// Created by ���� on 2022/1/26.
//
#include "mqtt.h"
#include "global.h"
#include "boardsComm.h"
#include "iwdg.h"
#include "stm_flash.h"

static char version[2]={"a1"};
uint8_t MODULE_IMEI[16] = {"0"};
uint8_t MODULE_ICCID[21] = {"0"};
uint8_t mqttTopic[30] = {"0"};//����IMEI��topic����������
MQTTPacket_connectData mqttConnectData = MQTTPacket_connectData_initializer;
Network_Location_t Network_Location;

/*
 * ��������ModuleClose
 * ���ܣ�4Gģ��ػ�
 * ���룺��
 * ���أ���
 */
static int ModuleClose(void) {
    char *pRet = NULL;
    uint8_t i = 0;
    if (SendATCommand("AT+QPOWD=0\r\n", "POWERED DOWN", WAIT_TIME_OUT) == 0) {
        DBG_PRINTF("CLOSE EC2000 Fail\r\n");
    } else {
        DBG_PRINTF("CLOSE OK:%s\r\n", msgRecBuff);
        return 0;
    }

    //�ػ�ʧ���ٹػ�һ��
    if (SendATCommand("AT+QPOWD=0\r\n", "POWERED DOWN", WAIT_TIME_OUT) == 0) {
        DBG_PRINTF("CLOSE EC2000 Fail\r\n");
        return -1;
    }
}

/*
 * ��������ModuleOpen
 * ���ܣ�4Gģ�鿪��
 * ���룺��
 * ���أ�0:�����ɹ���-1����ʧ��
 */
static int ModuleOpen(void) {
    HAL_GPIO_WritePin(ONOFF_EC200_GPIO_Port, ONOFF_EC200_Pin, GPIO_PIN_SET);
    HAL_Delay(600);//�ο�500
    HAL_GPIO_WritePin(ONOFF_EC200_GPIO_Port, ONOFF_EC200_Pin, GPIO_PIN_RESET);
    if (Wait_LTE_RDY(5) != 0) {
        return -1;
    }
    return 0;
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
    HAL_IWDG_Refresh(&hiwdg);//����������ʱ�ȴ�̫����ι��һ��
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
    if (pRet == 0) {
        DBG_PRINTF("msgRecBuff=%s\r\n", msgRecBuff);
        memcpy(&MODULE_IMEI[0], &msgRecBuff[2], 15);//ȥ��0D/0A
    } else {
        DBG_PRINTF("msgRecBuff=%s\r\n", msgRecBuff);
        memcpy(&MODULE_IMEI[0], pRet + 9, 15);//ȥ��0D/0A+AT+GSN\n
    }
    DBG_PRINTF("LTE_GET_IMEI Success:%s\r\n", &MODULE_IMEI[0]);
    return 0;
}

//��ȡICCID��
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

//�ж��ź�����
int GET_Signal_Quality(void) {
    char *pRet = 0;
    char *pRetT = 0;//�ָ���
    char buffer[5] = {0};
    pRet = SendATCommand("AT+CSQ\r\n", "OK", WAIT_TIME_IN);
    if (pRet == 0) {
        return -1;
    }

    if (strstr((char *) pRet, "99,99") != 0) {//99,99�����źŹ���
        return -1;
    }

    pRet = strstr(msgRecBuff, "+CSQ:");
    if (pRet == 0) {
        DBG_PRINTF("sig buffer:%s\r\n", msgRecBuff);
        return -1;
    }
    pRet += 6;//ת������λ��
    DBG_PRINTF("pRet buffer:%s\r\n", pRet);
    pRetT = strstr(pRet, ",");
    DBG_PRINTF("pRetT buffer:%s\r\n", pRetT);
    if ((pRetT - pRet) == 3) {
        memcpy(buffer, pRet, 2);
    } else if ((pRetT - pRet) == 2) {
        memcpy(buffer, pRet, 2);
    } else {
        return -1;
    }
    DBG_PRINTF("sig buffer:%s\r\n", buffer);
    return atoi(buffer);
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
            DBG_PRINTF(" No  RDY \r\n");
        } else {
            DBG_PRINTF(" RDY Come\r\n");
            return 0;
        }
        HAL_Delay(1000);
    }
    DBG_PRINTF(" Wait_LTE_RDY Timeout:%s\r\n", msgRecBuff);
    return -1;
}

int Wait_Signal_RDY(uint8_t time) {
    int signalValue = 0;
    while (--time) {
        signalValue = GET_Signal_Quality();
        if (signalValue == -1) {
            DBG_PRINTF(" No  Signal \r\n");
        } else {
            DBG_PRINTF(" Signal OK:%d\r\n", signalValue);
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
    mqttConnectData.clientID.cstring =(char*) MODULE_IMEI;
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
        if (ModuleOpen() != 0) {
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
    strcat(mqttTopic, &MODULE_IMEI[7]);//ƴ�Ӻ�8λ����:8-15
    DBG_PRINTF("mqttTopic:%s\r\n", mqttTopic);
    return 0;
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
 * ��������GetCellLoc
 * ���ܣ���ȡ��λ����
 * ���룺��
 * ���أ���
 */
int GetCellLoc(Network_Location_t *cellInfo) {
    uint8_t *ptr, ptrNext, dx;
    uint8_t posX = 0;//cell����λ��
    uint8_t posY = 0;//cell��һ������λ��
    char cellID[10] = {0};
    char TAC[10] = {0};
    if (SendATCommand("AT+QENG=\"servingcell\"\r\n", "OK", WAIT_TIME_OUT) == 0) {
        DBG_PRINTF("QENG :%s\r\n", msgRecBuff);
        return -1;
    }

    ptr = strstr((const char *) msgRecBuff, "+QENG:");
    if (ptr == 0) {
        return -1;
    }

    posX = MQTT_Comma_Pos(ptr, 6);
    posY = MQTT_Comma_Pos(ptr, 7);
    if (posX != 0XFF && posY != 0XFF) {
        memcpy(cellID, ptr + posX, (posY - posX - 1));
        DBG_PRINTF("cell16 id is:%s\r\n", cellID);
        cellInfo->sCellID = hexToDec(cellID);
        DBG_PRINTF("cell10 id is:%lu\r\n", cellInfo->sCellID);
    }

    posX = MQTT_Comma_Pos(ptr, 12);
    posY = MQTT_Comma_Pos(ptr, 13);
    if (posX != 0XFF && posY != 0XFF) {
        memcpy(TAC, ptr + posX, (posY - posX - 1));
        DBG_PRINTF("TAC16 id is:%s\r\n", TAC);
        cellInfo->sLac = hexToDec(TAC);
        DBG_PRINTF("TAC10 id is:%lu\r\n", cellInfo->sLac);
    }

    posX = MQTT_Comma_Pos(ptr, 4);
    if (posX != 0XFF) {
        cellInfo->sMcc = MQTT_Str2num(ptr + posX, &dx);
        DBG_PRINTF("sMcc is:%d\r\n", cellInfo->sMcc);
    }

    posX = MQTT_Comma_Pos(ptr, 5);
    if (posX != 0XFF) {
        cellInfo->sMnc = MQTT_Str2num(ptr + posX, &dx);
        DBG_PRINTF("sMnc is:%d\r\n", cellInfo->sMnc);
    }

    posX = MQTT_Comma_Pos(ptr, 7);
    if (posX != 0XFF) {
        cellInfo->iBsic = MQTT_Str2num(ptr + posX, &dx);
        DBG_PRINTF("iBsic is:%d\r\n", cellInfo->iBsic);
    }

    posX = MQTT_Comma_Pos(ptr, 8);
    if (posX != 0XFF) {
        cellInfo->nArfcn = MQTT_Str2num(ptr + posX, &dx);
        DBG_PRINTF("nArfcn is:%d\r\n", cellInfo->nArfcn);
    }

    posX = MQTT_Comma_Pos(ptr, 14);
    if (posX != 0XFF) {
        cellInfo->iRxLev = MQTT_Str2num(ptr + posX, &dx);
        DBG_PRINTF("iRxLevis:%d\r\n", cellInfo->iRxLev);
    }
    DBG_PRINTF("QCELLLOC:%s\r\n", msgRecBuff);
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
    char lenTemp[4] = {0};
    sprintf(lenTemp, "%d", strlen(payload));

    if (strlen(payload) == 0) {//���ַ���ֱ���˳�
        return -1;
    }

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

    if (SendATCommand(buf, "+QMTSUB: 0,1,0,2", WAIT_TIME_OUT) == 0) {
        DBG_PRINTF("sub error:%s\r\n", msgRecBuff);
        return -1;
    }
    return 0;
}

/*
 * ��������MQTTClient_checkMessage
 * ���ܣ�����յ�����Ϣ�Ƿ���ȷ
 * ���룺��
 * ���أ���
 */
char *MQTTClient_checkMessage(char *message, char *topic) {
    //����У��ͨ��,//0X5A="Z";0X4e="N";
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

    ptr = strstr(ptr, ",");
    ptr += 2;//�ַ�����ʼλ��

    //���ҷָ���λ��
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

    if (CheckXorAndMod(ptr, (ptrN - ptr)) != checkValue) {
        DBG_PRINTF("%d\r\n", checkValue);
        return NULL;
    }
    *(++ptrZ) = 0;
    return ptr;
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
 * ��������MQTClient_loc
 * ���ܣ���ѯ��վ���ź���Ϣ�����
 * ���룺��
 * ���أ���
 */
int MQTClient_sendLoc() {
    char sigTemp[4] = {0};

    char sMcc[4] = {0};
    char sMnc[4] = {0};
    char TacTemp[10] = {0};
    char cellIDTemp[10] = {0};
    char ibsicTemp[6] = {0};
    char iRxLevTemp[6] = {0};
    char iRxLevSubTemp[6] = {0};
    char nArfcnTemp[6] = {0};
    char checkTemp[3] = {0};
    int8_t signalValue;
    uint8_t checkValue;

    GetCellLoc(&Network_Location);//
    signalValue = GET_Signal_Quality();
    DBG_PRINTF("signalValue:%d\r\n", signalValue);
    memset(msgSendBuff, 0, MSG_SEND_LEN);
    if (signalValue != -1) {
        sprintf(sigTemp, "%d", signalValue);//��ӡ���ݻ��ַ���
    }
    sprintf(sMcc, "%d", Network_Location.sMcc);
    sprintf(sMnc, "%d", Network_Location.sMnc);
    sprintf(TacTemp, "%lu", Network_Location.sLac);
    sprintf(cellIDTemp, "%lu", Network_Location.sCellID);
    sprintf(ibsicTemp, "%d", Network_Location.iBsic);
    sprintf(iRxLevTemp, "%d", Network_Location.iRxLev);
    sprintf(iRxLevSubTemp, "%d", Network_Location.iRxLevSub);
    sprintf(nArfcnTemp, "%d", Network_Location.nArfcn);

    strcpy(msgSendBuff, "LOC:");
    strcat(msgSendBuff, sMcc);
    strcat(msgSendBuff, ",");
    strcat(msgSendBuff, sMnc);
    strcat(msgSendBuff, ",");
    strcat(msgSendBuff, TacTemp);
    strcat(msgSendBuff, ",");
    strcat(msgSendBuff, cellIDTemp);
    strcat(msgSendBuff, ",");
    strcat(msgSendBuff, ibsicTemp);
    strcat(msgSendBuff, ",");
    strcat(msgSendBuff, iRxLevTemp);
    strcat(msgSendBuff, ",");

    strcat(msgSendBuff, iRxLevSubTemp);
    strcat(msgSendBuff, ",");
    strcat(msgSendBuff, nArfcnTemp);
    strcat(msgSendBuff, ":CSQ");
    strcat(msgSendBuff, sigTemp);
    strcat(msgSendBuff, "N");

    checkValue = CheckXorAndMod(msgSendBuff, strlen(msgSendBuff) - 1);
    sprintf(checkTemp, "%d", checkValue);
    strcat(msgSendBuff, checkTemp);
    strcat(msgSendBuff, "Z");//ģʽ

    if (MQTTClient_pubMessage(mqttTopic, msgSendBuff) == 0) {
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
    static uint8_t openError = 0;
    static uint8_t loginError = 0;
    //�쳣������ƣ�����������Σ����µ�¼����
    if (openError > 3) {
        openError = 0;
        MCU_STATUS.MQTT_STATUS = MQTT_ERROR;
    }
    if (loginError > 3) {
        loginError = 0;
        MCU_STATUS.MQTT_STATUS = MQTT_RESTART;
    }

    switch (MCU_STATUS.MQTT_STATUS) {
        case MQTT_OFFLINE:
            DBG_PRINTF("MQTT_OFFLINE\r\n");
            if (MQTTClient_init() != 0) {
                openError++;//����ʧ�ܴ�����1
                return;
            }
            MQTTParam_init();
            MCU_STATUS.MQTT_STATUS = MQTT_LOGIN;
            break;

        case MQTT_LOGIN:
            if (MQTTClient_disconnect() == 0)//�������ã���ֹ����error
            {
                DBG_PRINTF("disconnect ok\r\n");
            }
            if (MQTTClient_connect(mqttConnectData) != 0) {
                loginError++;
                return;
            }

            if (MQTTClient_subMessage(SUBSCRIBE_TOPIC) != 0) {
                loginError++;
                return;
            }
            MQTTClient_pubMessage(mqttTopic, version);
            openError = 0;
            loginError = 0;
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
                if (strstr(ptr, "update") != 0) {
                    boardsDownFlag = 0;//����ȡ��ת��ָ��
                    if(strstr(ptr, version) == 0) {//�汾�Ų�һ��
                        //��Ҫ������дflash������all_restart
                        __disable_irq();
                        //дflash
                        STMFLASH_Write(FLASH_InfoAddress, (uint16_t *) msgRecBuff, 50);//100byte
                        __enable_irq();
                        //�Ƿ����д��У��
                        DBG_PRINTF("write flash ota\r\n");
                        MCU_STATUS.MQTT_STATUS = MQTT_ALL_RESTART;
                    }
                }
            }

            if (Task_timer.publishTimer1s == 0) {
                Task_timer.publishTimer1s = PUBLISH_INTERVAL;

                if (MQTClient_sendLoc() == 0) {
                    boardSendOkFlag = 1;
                }
            }

            if (io2DownFlag == 1) {
                io2DownFlag = 0;
                uint8_t ioValue = 0;
                uint8_t i = 0;
                for (i = 0; i < 8; i++) {
                    HAL_Delay(2);
                    ioValue += HAL_GPIO_ReadPin(IO2_GPIO_Port, IO2_Pin);
                }
                if (ioValue < 2) {
                    MQTTClient_pubMessage(mqttTopic, "ALERTD:1N9Z");
                }
            }
            break;
        case MQTT_RESTART:
            //�ظ���¼��������һ��
            ModuleClose();
            HAL_Delay(2000);
            MCU_STATUS.MQTT_STATUS = MQTT_OFFLINE;
            break;

        case MQTT_ERROR:
            //ģ����ϲ����У���ֹƵ��������ģ��
            break;
        case MQTT_ALL_RESTART:
            ModuleClose();
            HAL_Delay(2000);
            SoftReset();
            break;
        default:
            break;
    }
}

