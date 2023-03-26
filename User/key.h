#ifndef __KEY_H
#define __KEY_H	 
#include "sys.h"

#define KEY0  GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_12)//��ȡ����0
#define KEY1  GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_13)//��ȡ����1

#define	VIBRATE PBin(9)//�𶯴�����

#define	BEEP PCout(13)//������
#define	GSM_LED PCout(14)//GSMָʾ��
#define	GPS_LED PCout(15)//GPSָʾ��
#define	FD_LED PAout(0) //����ģʽָʾ��

void KEY_Init(void);//IO��ʼ��
u8 KEY_Scan(u8 mode);  	//����ɨ�躯��					    
#endif
