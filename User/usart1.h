#ifndef __usart1_H
#define __usart1_H	 
#include "stm32f10x.h"

#define USART1_RXBUFF_SIZE   200 

extern unsigned int RxCounter;          //�ⲿ�����������ļ����Ե��øñ���
extern char Usart1RecBuf[USART1_RXBUFF_SIZE]; //�ⲿ�����������ļ����Ե��øñ���

extern u8 gsm_rev_start;
extern u8 gsm_rev_okflag;      //gsm�����־

void Usart1_Init(u32 bound);
void Uart1_SendStr(char*SendBuf);
void uart_send(unsigned char *bufs,unsigned char len);
		 				    
#endif


