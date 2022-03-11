//
// Created by 咚咚 on 2022/1/25.
//
#include "stm32f0xx_hal.h"
#include "global.h"
/****************宏定义********************/

/****************定义变量********************/
const uint8_t table_week[12]={0,3,3,6,1,4,6,2,5,0,3,5}; //月修正数据表
//平年的月份日期表
const uint8_t mon_table[12]={31,28,31,30,31,30,31,31,30,31,30,31};
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
uint8_t boardsGetTimeFlag=0;     
uint32_t boardsRxSize=0;

uint8_t io2DownFlag=0;
uint8_t rstDownFlag=0;

uint32_t net_time = 0;

/*
 * 函数名：FindStrFroMem
 * 功能：从接收的数组中查找指定的字符串
 *      如果都是字符串传输可替换为strstr()函数，否则传输数据有0存在会打断
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
 * 功能：将数转化为对应的字符串值
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

//判断是否闰年
uint8_t Is_Leap_Year(uint16_t year)
{
    if(year%4==0) //必须能被4整除
    {
        if(year%100==0)
        {
            if(year%400==0)return 1;
            else return 0;
        }else return 1;
    }else return 0;
}

/*
 * 函数名：time_stamp_Set
 * 功能：//获取时间戳
 * 输入：syear，smon.sday,hour,min,sec
 * 返回：生成的字符串地址
 */

uint32_t time_stamp_Set(uint16_t syear,uint8_t smon,uint8_t sday,uint8_t hour,uint8_t min,uint8_t sec)
{
    uint16_t t;
    uint32_t seccount=0;
    if(syear<1970||syear>2099)return 1;
    for(t=1970;t<syear;t++) //把所有年份的秒钟相加
    {
        if(Is_Leap_Year(t))seccount+=31622400;//闰年的秒钟数
        else seccount+=31536000; //平年的秒钟数
    }
    smon-=1;
    for(t=0;t<smon;t++) //把前面月份的秒钟数相加
    {
        seccount+=(uint32_t)mon_table[t]*86400;//月份秒钟数相加
        if(Is_Leap_Year(syear)&&(t==1))seccount+=86400;//闰年2月份增加一天的秒钟数
    }
    seccount+=(uint32_t)(sday-1)*86400;//把前面日期的秒钟数相加
    seccount+=(uint32_t)hour*3600;//小时秒钟数
    seccount+=(uint32_t)min*60; //分钟秒钟数
    seccount+=sec;//最后的秒钟加上去
    return seccount;
}
/*
 * 函数名：SoftReset
 * 功能：软件复位，接受到OTA指令后执行
 * 输入：无
 * 返回：无
 */
void SoftReset(void)
{
    __set_PRIMASK(1);
    HAL_NVIC_SystemReset();
}

/*
 * 函数名：MQTT_Comma_Pos
 * 功能：查找buffer里的指定位置数据
 * 输入：buf:接受数据地址，cx:第几个分隔符
 * 返回：地址差值
 */
uint8_t MQTT_Comma_Pos(uint8_t *buf,uint8_t cx)
{
    uint8_t *p=buf;
    while(cx)
    {
        if(*buf=='*'||*buf<' '||*buf>'z')return 0XFF;//遇到'*'或者非法字符,则不存在第cx个逗号
        if(*buf==',')cx--;
        buf++;
    }
    return buf-p;   //返回差值，
}

/*
 * 函数名：NMEA_Pow
 * 功能：各单位数乘以自己的位数
 * 输入：m:默认10，n:位数，乘以几次10
 * 返回：数值
 */
uint32_t NMEA_Pow(uint8_t m,uint8_t n)
{
    uint32_t result=1;
    while(n--)result*=m;
    return result;
}

/* 返回ch字符在sign数组中的序号 */
int getIndexOfSigns(char ch) {
    if (ch >= '0' && ch <= '9') {
        return ch - '0';
    }
    if (ch >= 'A' && ch <= 'F') {
        return ch - 'A' + 10;
    }
    if (ch >= 'a' && ch <= 'f') {
        return ch - 'a' + 10;
    }
    return -1;
}

long hexToDec(char *source)
{
    long sum = 0;
    long t = 1;
    int i, len;

    len = strlen(source);
    for(i=len-1; i>=0; i--)
    {
        sum += t * getIndexOfSigns(*(source + i));
        t *= 16;
    }

    return sum;
}

/*
 * 函数名：MQTT_Str2num
 * 功能：软件复位，接受到OTA指令后执行
 * 输入：buf:接受数据地址，dx:第几个分隔符
 * 返回：数值
 */
int MQTT_Str2num(uint8_t *buf,uint8_t*dx)
{
    uint8_t *p=buf;
    uint32_t ires=0,fres=0;
    uint8_t ilen=0,flen=0,i;//整数长度，小数长度，i
    uint8_t mask=0;//小数和符号标志位
    int res;
    while(1) //得到整数和小数的长度
    {
        if(*p=='-'){mask|=0X02;p++;}//是负数
        if(*p==','||(*p=='*'))break;//遇到结束了
        if(*p=='.'){mask|=0X01;p++;}//遇到小数点了
        else if(*p>'9'||(*p<'0'))   //有非法字符
        {
            ilen=0;
            flen=0;
            break;
        }
        if(mask&0X01)flen++;
        else ilen++;
        p++;
    }
    if(mask&0X02)buf++; //去掉负号
    for(i=0;i<ilen;i++) //得到整数部分数据
    {
        ires+=NMEA_Pow(10,ilen-1-i)*(buf[i]-'0');
    }
    if(flen>2)flen=2;   //最多取2位小数
    *dx=flen;           //小数点位数
    for(i=0;i<flen;i++) //得到小数部分数据
    {
        fres+=NMEA_Pow(10,flen-1-i)*(buf[ilen+1+i]-'0');
    }
    res=ires*NMEA_Pow(10,flen)+fres;
    if(mask&0X02)res=-res;
    return res;
}