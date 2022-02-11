//
// Created by 咚咚 on 2022/1/25.
//
#include "stm32f0xx_hal.h"
#include "global.h"
/****************宏定义********************/


/****************结构体********************/
MQTT_COMPONENTS MCU_STATUS;//状态机切换
ProcessTask_timer Task_timer;
/****************数组缓存区和信号量********************/
uint8_t msgSendBuff[MSG_SEND_LEN] = {0}; //发送缓存区
uint8_t msgRecBuff[MSG_REC_LEN] = {0}; //接收缓存区
uint8_t msgRxFlag=0;       //接收完成标记
uint32_t msgRxSize=0;

uint8_t boardsSendBuff[BOARDS_SEND_LEN] = {0}; //发送缓存区
uint8_t boardsRecBuff[BOARDS_REC_LEN] = {0}; //接收缓存区
uint8_t boardsRxFlag=0;       //接收完成标记
uint8_t boardsDownFlag=0;       //下发标志
uint32_t boardsRxSize=0;

uint8_t io2DownFlag=0;
/*
 * 函数名：FindStrFroMem
 * 功能：从接收的数组中查找指定的字符串
 * 输入：buf，buflen，str
 * 返回：字符串地址，null代表未找到
 */
char* FindStrFroMem(char *buf, uint16_t buflen, char *str)
{
    uint8_t len = strlen(str);
    uint8_t ret;
    uint32_t temp = 0;
    if (len > buflen)
        return NULL;

    while (1) {
        ret = memcmp(buf + temp, str, len);
        if (ret == 0) {
            return (buf + temp);
        }
        temp++;
        if (buflen < temp)
            return NULL;
    }
    return NULL;
}

/*
 * 函数名：Int2String
 * 功能：将数组转化为对应的字符串值
 * 输入：num，str
 * 返回：生成的字符串地址
 */
char* Int2String(int num,char *str)//10进制
{
    int i = 0;//指示填充str
    int j = 0;//确定开始调整的位置
    if(num<0)//如果num为负数，将num变正
    {
        num = -num;
        str[i++] = '-';
    }
    //转换
    do
    {
        str[i++] = num%10+48;//取num最低位 字符0~9的ASCII码是48~57；简单来说数字0+48=48，ASCII码对应字符'0'
        num /= 10;//去掉最低位
    }while(num);//num不为0继续循环

    str[i] = '\0';

    //确定开始调整的位置
    if(str[0]=='-')//如果有负号，负号不用调整
    {
        j = 1;//从第二位开始调整
        ++i;//由于有负号，所以交换的对称轴也要后移1位
    }
    //对称交换
    for(;j<i/2;j++)
    {
        //对称交换两端的值 其实就是省下中间变量交换a+b的值：a=a+b;b=a-b;a=a-b;
        str[j] = str[j] + str[i-1-j];
        str[i-1-j] = str[j] - str[i-1-j];
        str[j] = str[j] - str[i-1-j];
    }
    return str;//返回转换后的值
}

/*
 * 函数名：CheckXorAndMod
 * 功能：‘N’之前的所有字符（不包含‘N’）的ASCII码依次进行异或运算得到的char类型值，再异或上‘N’之前（不包含‘N’）所有字符的长度，再对100取模，得到整型校检码。
 * 输入：num，str
 * 返回：生成的字符串地址
 */
uint8_t CheckXorAndMod(uint8_t *data, uint32_t len)
{
    uint32_t i = 0;
    uint8_t result = 0;
    uint8_t *buf = data;
    for(i=0; i<len; i++)
    {
        result ^= *(buf++);
    }
    result^=len;
    result%=100;
    return result;
}