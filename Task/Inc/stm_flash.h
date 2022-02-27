#ifndef __STMFLASH_H__
#define __STMFLASH_H__

/* ����ͷ�ļ� ----------------------------------------------------------------*/
#include "stm32f0xx_hal.h"

/* ���Ͷ��� ------------------------------------------------------------------*/
/* �궨�� --------------------------------------------------------------------*/
/************************** STM32 �ڲ� FLASH ���� *****************************/
#define STM32_FLASH_SIZE 64  //��ѡSTM32��FLASH������С(��λΪK)
#define STM32_FLASH_WREN 1    //ʹ�� FLASH д��(0��������;1��ʹ��)
#define STM32_FLASH_BASE 0x8000000
#define FLASH_WAITETIME  	50000          	//FLASH�ȴ���ʱʱ��
//!!!��Ϊ�忨û�����spi����˴������23k
#define FLASH_AppAddress 0x8004000 //app��ʼ��ַ��16k��ʼ
#define FLASH_FileAddress 0x8009C00 //�ļ���ַ��40��ʼ
#define FLASH_InfoAddress 0x800FC00 //�洢������Ϣ��63k��ʼ

/* ��չ���� ------------------------------------------------------------------*/
/* �������� ------------------------------------------------------------------*/
extern void    FLASH_PageErase(uint32_t PageAddress);
extern void STMFLASH_Init(void) ;
uint8_t STMFLASH_Readbyte ( uint32_t faddr );
uint16_t STMFLASH_ReadHalfWord(uint32_t faddr);		  //��������  

void STMFLASH_Write( uint32_t WriteAddr, uint16_t * pBuffer, uint16_t NumToWrite );		//��ָ����ַ��ʼд��ָ�����ȵ�����
void STMFLASH_Read( uint32_t ReadAddr, uint16_t * pBuffer, uint16_t NumToRead );   	//��ָ����ַ��ʼ����ָ�����ȵ�����



#endif /* __STMFLASH_H__ */

/******************* (C) COPYRIGHT 2015-2020 ӲʯǶ��ʽ�����Ŷ� *****END OF FILE****/
