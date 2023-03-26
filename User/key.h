#ifndef __KEY_H
#define __KEY_H	 
#include "sys.h"

#define KEY0  GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_12)//读取按键0
#define KEY1  GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_13)//读取按键1

#define	VIBRATE PBin(9)//震动传感器

#define	BEEP PCout(13)//蜂鸣器
#define	GSM_LED PCout(14)//GSM指示灯
#define	GPS_LED PCout(15)//GPS指示灯
#define	FD_LED PAout(0) //防盗模式指示灯

void KEY_Init(void);//IO初始化
u8 KEY_Scan(u8 mode);  	//按键扫描函数					    
#endif
