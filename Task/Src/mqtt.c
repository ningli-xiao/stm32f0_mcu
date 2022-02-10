//
// Created by 咚咚 on 2022/1/26.
//
#include "mqtt.h"
#include "global.h"


static uint8_t MODULE_IMEI[20] = {"0"};
static uint8_t MODULE_ICCID[25] = {"0"};

static uint8_t mqttTopic[20] = {"0"};//根据IMEI和topic构建新主题

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
    HAL_GPIO_WritePin(ONOFF_EC200_GPIO_Port, ONOFF_EC200_Pin, GPIO_PIN_RESET);
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
    HAL_GPIO_WritePin(ONOFF_EC200_GPIO_Port, ONOFF_EC200_Pin, GPIO_PIN_RESET);
    HAL_Delay(1500);
    HAL_GPIO_WritePin(ONOFF_EC200_GPIO_Port, ONOFF_EC200_Pin, GPIO_PIN_RESET);
}

void UART_SendData(char *pdatabuf) {
    if (HAL_UART_Transmit(&huart2, pdatabuf, 100, 1000) != HAL_OK) {
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
    uint8_t *pRet = NULL;
    uint8_t i = 0;
    if (SendATCommand("AT+GSN\r\n", "OK", WAIT_TIME_IN) == 0) {
        DBG_PRINTF("LTE_GET_Imei Fail\r\n");
        return -1;
    }
    HAL_Delay(100);
    pRet = FindStrFroMem((char *) msgRecBuff, msgRxSize, "AT+GSN");
    DBG_PRINTF("pRet=%s\r\n", pRet);
    MODULE_IMEI[0] = pRet[9] - 0x30;
    for (i = 1; i < 8; i++) {
        MODULE_IMEI[i] = ((pRet[8 + i * 2] - 0x30) << 4) | (pRet[9 + i * 2] - 0x30);
    }
    DBG_PRINTF("LTE_GET_IMEI Success\r\n");
    return 0;
}

//获取ICCID号
int GET_ICCID(void) {
    uint8_t *pRet = 0;
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

int Wait_LTE_RDY(uint8_t time)
{
    while(--time)
    {
        if(SendATCommand(NULL, "RDY",WAIT_TIME_IN) == 0)
        {
            printf(" No  RDY \r\n");
        }
        else
        {
            printf(" RDY Come\r\n");
            return 0;
        }
        HAL_Delay(10);
    }
    printf(" Wait_LTE_RDY Timeout:%s\r\n",msgRecBuff);
    return -1;
}

/*
 * 函数名：SendDataPacket
 * 功能：发送数据包
 * 输入：无
 * 返回：无
 */
static int SendDataPacket(uint8_t value)
{

}

/*
 * 函数名：SendHeartPacket
 * 功能：发送心跳包
 * 输入：无
 * 返回：无
 */
static int SendHeartPacket(void)
{

}

/*
 * 函数名：SendLoginPacket
 * 功能：发送注册包
 * 输入：无
 * 返回：无
 */
static int SendLoginPacket(void)
{

}
int Wait_Signal_RDY(uint8_t time)
{
    while(--time)
    {
        //if(LTE_Signal_Quality() != 0)
        if(0)
        {
            printf(" No  Signal \r\n");
        }
        else
        {
            printf(" Signal OK\r\n");
            return 0;
        }
        HAL_Delay(500);
    }

    printf(" Wait_Signal_RDY Timeout\r\n");
    return -1;
}

int Wait_Send_Login_Packet_RDY(uint8_t time)
{
    while(--time)
    {
        if(SendLoginPacket() != 0)
        {
            DBG_PRINTF(" Send_Login_Packet  Faile \r\n");
        }
        else
        {
            DBG_PRINTF(" Send_Login_Packet  Success \r\n");
            return 0;
        }
        HAL_Delay(1000);
    }

    printf(" Send_Login_Packet  Timeout \r\n");
    return -1;
}

int Wait_Send_data_Packet_RDY(uint8_t time,uint8_t num)
{
    while(--time)
    {
        if(SendDataPacket(num) != 0)
        {
            DBG_PRINTF(" Send_data_Packet  Fail \r\n");
        }
        else
        {
            DBG_PRINTF(" Send_data_Packet  Success \r\n");
            return 0;
        }
        HAL_Delay(1000);
    }
    DBG_PRINTF(" Send_data_Packet Timeout \r\n");
    return -1;
}

/*
 * 函数名：mqttTask
 * 功能：mqtt任务执行，状态机方式
 * 输入：无
 * 返回：无
 */
void mqttTask()
{
    static uint8_t restart_count=0;
    switch (MCU_STATUS.MQTT_STATUS) {
        case MQTT_OFFLINE:
            DBG_PRINTF("MQTT_OFFLINE\r\n");
            if(SendATCommand("ati\r\n","OK",WAIT_TIME_IN)==0){
                ModuleOpen();
            }
            Wait_LTE_RDY(3);
            if(SendATCommand("ati\r\n","OK",WAIT_TIME_IN)!=0){
                DBG_PRINTF("run 0:%s\r\n",msgRecBuff);
            }

            if(SendATCommand("at+sgsw\r\n","OK",WAIT_TIME_IN)!=0){
                DBG_PRINTF("run 1:%s\r\n",msgRecBuff);
            }

            if(SendATCommand("at+sgsw\r\n","OK",WAIT_TIME_IN)!=0){
                DBG_PRINTF("run 2:%s\r\n",msgRecBuff);
            }

            MCU_STATUS.MQTT_STATUS = MQTT_LOGIN;
            break;

        case MQTT_LOGIN:

            DBG_PRINTF("MQTT_LOGIN\r\n");
            break;

        case MQTT_ONLINE:
            DBG_PRINTF("MQTT_ONLINE\r\n");
            break;
        default:
            DBG_PRINTF("not run well\r\n");
            break;
    }
}

