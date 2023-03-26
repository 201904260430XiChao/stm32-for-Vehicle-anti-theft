#include "stm32f10x.h"
#include "usart1.h"
#include "usart3.h"
#include "delay.h"
#include "key.h"
#include "stdio.h"
#include "string.h"
#include "stmflash.h"
#include "GPS.h"

#define FLASH_SAVE_ADDR  ((u32)0x0800F000) 				//设置FLASH 保存地址(必须为偶数)

#define STM32_RX1_BUF       Usart1RecBuf 
#define STM32_Rx1Counter    RxCounter
#define STM32_RX1BUFF_SIZE  USART1_RXBUFF_SIZE

#define STM32_RX3_BUF       Usart3RecBuf 
#define STM32_Rx3Counter    Rx3Counter
#define STM32_RX3BUFF_SIZE  USART3_RXBUFF_SIZE

char   PhoneNumber[11];//手机号码
char ConversionNum[44];//手机号码转码后存放数组

u8 SendFlag=0;						//不同报警标志，发送不同内容短信
u8 GpsInitOkFlag=0; 			//GPS定位接收成功标志
u8 SOS_Flag=0;						//一键SOS标志
u8 SetPhoneNumFlag=0;     //1：设置成功，2设置失败
u8 FangDao=0;             //防盗标志
u8 VibrateFlag=0;         //发生震动标志

GPS_INFO   GPS;  //GPS信息结构体

extern unsigned char rev_start;
extern unsigned char rev_stop;
extern unsigned char gps_flag;
u8 GPS_rx_flag = 0;

void gsm_atcmd_send(char *at)//GSM AT指令发送函数
{
    unsigned short waittry;//延时变量
    do
    {
        gsm_rev_start = 0;//接收开始标志清零
        gsm_rev_okflag = 0;//接收完成标志清零
        waittry = 0;//延时变量清零
        uart_send(at,0xFF);//串口发送内容
        while(waittry ++ < 3000)//进入while延时
        {
            if (gsm_rev_okflag == 1)//等待GSM返回OK
            {
                return;//返回出去
            }
            DelayMs(1);
        }
    }
    while(gsm_rev_okflag == 0);
}

void gsm_init(void)//gsm初始化
{
    gsm_atcmd_send("AT\r\n");//测试用的
    DelayMs(1000);
    gsm_atcmd_send("AT+CSCS=\"UCS2\"\r\n");//设置为unicode编码
    DelayMs(1000);
    gsm_atcmd_send("AT+CMGF=1\r\n");//设置为文本模式
    DelayMs(1000);
    gsm_atcmd_send("AT+CNMI=2,1\r\n");//来短信时提示，并存储到模块内存
	  DelayMs(1000);
	  gsm_atcmd_send("AT+CMGD=1,4\r\n");//清除短信
	  DelayMs(1000);
	  gsm_atcmd_send("AT+CSMP=17,0,2,25\r\n");//设置短信保留为5分钟，发送中文
	  DelayMs(1000);
}

/*
*number 为对方手机号
*/
void gsm_send_msg(const char*number,char * content)
{
    u8 len;
    unsigned char  gsm_at_txbuf[60];//GSM AT命令缓存区
	
    memset(gsm_at_txbuf, 0, 60);//缓存清零
    strncpy((char *)gsm_at_txbuf,"AT+CMGS=\"",9);//将AT+CMGS=\"，复制到gsm_at_txbuf
    memcpy(gsm_at_txbuf + 9, number, 44);//将手机号码复制到AT+CMGS=\"之后
    len = strlen((char *)gsm_at_txbuf);//获取gsm_at_txbuf字符串长度
    gsm_at_txbuf[len] = '"'; // AT+CMGS=\"12345678901\"
    gsm_at_txbuf[len + 1] = '\r';
    gsm_at_txbuf[len + 2] = '\n';//gsm_at_txbuf最终的格式"AT+CMGS=\"手机号码\"\r\n"

    uart_send(gsm_at_txbuf,0xFF);//发送需要接受短信的手机号码
    DelayMs(1000);

    uart_send(content,0xFF);  //发短信内容
    DelayMs(100);

    printf("%c",0x1a);     //发送结束符号
    DelayMs(10);
}

void PhoneNumTranscoding(void)//手机号转码
{
	  u8 i=0;
		for(i=0;i<11;i++)//发送中文短信手机号必须转码，前面需要加上003
		{
				ConversionNum[i*4+0] = '0';
			  ConversionNum[i*4+1] = '0';
			  ConversionNum[i*4+2] = '3';
			  ConversionNum[i*4+3] = PhoneNumber[i];
		}
}

void AddressTranscoding(char *str1,char *str2)//经纬度坐标转码
{
	  u8 i=0,len;
	  char *buf = str1;
	  len = strlen(buf);//获取字符串长度
	
		for(i=0; i < 3; i++)//小数点前3位
		{
				if(buf[i] != ' ')
				{    
            *str2++ = '0';
					  *str2++ = '0';
					  *str2++ = '3';
					  *str2++ = buf[i];
				}
		}
		
		*str2++ = '0';*str2++ = '0';//小数点
		*str2++ = '2';*str2++ = 'E';

		i++;
		
		for(;i < len-1; i++)//小数点后6位
		{
				*str2++ = '0';
				*str2++ = '0';
				*str2++ = '3';
				*str2++ = buf[i];
		}
		*str2 = '\0';
}

void KeySettings(void)//按键设置函数
{
	  unsigned char keynum = 0;
	
	  keynum = KEY_Scan(0);//获取按键值
}

void CheckNewMcu(void)  // 检查是否是新的单片机，是的话清空存储区，否则保留
{
	  u8 comper_str[6];
		
	  STMFLASH_Read(FLASH_SAVE_ADDR + 0x10,(u16*)comper_str,5);
	  comper_str[5] = '\0';
	  if(strstr((char *)comper_str,"STM32") == NULL)  //新的单片机
		{
			 STMFLASH_Write(FLASH_SAVE_ADDR + 0x10,(u16*)"STM32",5); //写入“STM32”，方便下次校验
			 DelayMs(50);
			 STMFLASH_Write(FLASH_SAVE_ADDR + 0x40,(u16*)"12345678910",11);//存入初始手机号
			 DelayMs(50);
	  }
		STMFLASH_Read(FLASH_SAVE_ADDR + 0x40,(u16*)PhoneNumber,11); //读出手机号
		PhoneNumTranscoding();
		DelayMs(100);
}

void ContentHandle(void)//短信内容处理，发送中文短信必须把内容转化为Unicode码
{
		char TXbuf[400],databuf[50];             //发送数据缓冲区
	  char tempbuf[11]; 
	
		memset(TXbuf,0, 400);   			//清空缓冲区
		memset(databuf,0,50);         //清空缓冲区
	

		gsm_send_msg(ConversionNum,TXbuf);//发送短信
		SendFlag=0;
}


void SMS_Receive(void)//短信接收
{
	  u8 len,i=0,k;
	  char  *presult1=0,*presult2=0;
	  char  *str1=0,*str2=0;
	  char  tempbuf[100],tempNum[11];
	  char  gsm_at_txbuf1[20];
	  char  SmsNum[3];

	  if(strstr(STM32_RX1_BUF,"5F00542F963276D7")!=NULL)
		{
				if(VibrateFlag==0&&SOS_Flag==0)
				{
					
						BEEP = 0;
				}
			  memset(STM32_RX1_BUF, 0, STM32_RX1BUFF_SIZE);//清除缓存
				STM32_Rx1Counter = 0;
			
	
        SendFlag=3;//发送短信标志
		}
	
		if(strstr(STM32_RX1_BUF,"517395ED963276D7")!=NULL)//接收到关闭防盗命令
		{
				if(VibrateFlag==0&&SOS_Flag==0)
				{
						BEEP = 1;
						DelayMs(200);
						BEEP = 0;
				}
			  memset(STM32_RX1_BUF, 0, STM32_RX1BUFF_SIZE);//清除缓存
				STM32_Rx1Counter = 0;
			
			
        
		}
		

		
	  if(strstr(STM32_RX1_BUF,"+CMTI:")!=NULL)        //接收到短信  +CMTI: "SM",1
		{
			  presult1 = strstr(STM32_RX1_BUF,"+CMTI:");
			  if(strstr(presult1,"\"SM\",") != NULL ) 
				{				
					  presult2 = strstr(presult1,"\"SM\",");	// presult2="SM",1
					  DelayMs(20);	
					  memset(gsm_at_txbuf1, 0, 20);//清空发送缓存区
					  memset(SmsNum, 0, 3);//清空缓存区
						strncpy(gsm_at_txbuf1,"AT+CMGR=",8);//将AT+CMGR="，复制到gsm_at_txbuf
					  memcpy(SmsNum,presult2 + 5,2);
						memcpy(gsm_at_txbuf1 + 8, SmsNum, 2);//AT+CMGR=1
						len = strlen(gsm_at_txbuf1);//获取gsm_at_txbuf字符串长度
						gsm_at_txbuf1[len] = '\r';
						gsm_at_txbuf1[len + 1] = '\n';//gsm_at_txbuf最终的格式"AT+CMGR=XXX\r\n"
						uart_send(gsm_at_txbuf1,0xFF);
				
					  memset(STM32_RX1_BUF, 0, STM32_RX1BUFF_SIZE);//清除缓存
						STM32_Rx1Counter = 0;
				}
		}
}

void SMS_Send(void)//短信发送
{ 
	  if(SendFlag!=0)//短信发送标志不等于0，说明要发短信
		{
				ContentHandle();
		}
		if(SetPhoneNumFlag==1)
		{
				SetPhoneNumFlag = 0;
			  gsm_send_msg(ConversionNum,"624B673A53F778018BBE7F6E6210529FFF01");//发送短信，手机号码设置成功！
		}
		if(SetPhoneNumFlag==2)
		{
				SetPhoneNumFlag = 0;
			  gsm_send_msg(ConversionNum,"624B673A53F778018BBE7F6E59318D25FF018BF760A8518D4ED47EC668385BF9624B673A53F77801FF01");//发送短信，手机号码设置失败！
		}
}

int main(void)
{
	char error_num = 0;
	u16 count;
	u16 alarm_delay=0;
	
	DelayInit();
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);	//设置NVIC中断分组2:2位抢占优先级，2位响应优先级
	CheckNewMcu();//检测是否为新的单片机
	KEY_Init(); //按键初始化
	GPS_rx_flag = 0;
	Usart1_Init(9600);
	USART3_Init(9600);

	GPS_rx_flag = 1;
  GSM_LED = 1;
	while(1)
	{
		  SMS_Receive();//短信接收
		  KeySettings();
		  if(FangDao==1)
			{
					FD_LED=1;
				  if(VIBRATE==1)
					{
							DelayMs(100);//滤除干扰信号
						  if(VIBRATE==1) 
							{
									if(VibrateFlag==0)
									{
											VibrateFlag=1;
										  BEEP = 1;//蜂鸣器响
											alarm_delay = 0;
										  SendFlag = 1;//发送短信标志
									}
							}
					}
					if(VibrateFlag==1)
					{
							if(alarm_delay++ >= 3000)//震动报警延时时间
						 {
								 VibrateFlag = 0;
								 if(SOS_Flag==0)BEEP = 0;//蜂鸣器关闭
								 alarm_delay = 0;
						 }
					}
			}
			else
			{
					FD_LED=0;
				  if(SOS_Flag==0)BEEP = 0;
				  VibrateFlag=0;
				  alarm_delay = 0;
			}
			if (rev_stop == 1 && count++>=1000)   //如果接收完一行
			{
					if (GPS_RMC_Parse(STM32_RX3_BUF, &GPS)) //解析GPRMC
					{
							error_num = 0;
							gps_flag = 0;
							rev_stop  = 0;
							GpsInitOkFlag=1;
						  GPS_LED = 1;//GPS指示灯亮起
					}
					else
					{
							if (error_num++ >= 3) //如果数据无效超过3次
							{
									error_num = 3;
									GpsInitOkFlag = 0;
                  GPS_LED = 0;//GPS指示灯熄灭
							}
							gps_flag = 0;
							rev_stop  = 0;
					}
					count=0;
			}
			SMS_Send();
			DelayMs(1);
	}
}
