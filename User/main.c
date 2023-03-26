#include "stm32f10x.h"
#include "usart1.h"
#include "usart3.h"
#include "delay.h"
#include "key.h"
#include "stdio.h"
#include "string.h"
#include "stmflash.h"
#include "GPS.h"

#define FLASH_SAVE_ADDR  ((u32)0x0800F000) 				//����FLASH �����ַ(����Ϊż��)

#define STM32_RX1_BUF       Usart1RecBuf 
#define STM32_Rx1Counter    RxCounter
#define STM32_RX1BUFF_SIZE  USART1_RXBUFF_SIZE

#define STM32_RX3_BUF       Usart3RecBuf 
#define STM32_Rx3Counter    Rx3Counter
#define STM32_RX3BUFF_SIZE  USART3_RXBUFF_SIZE

char   PhoneNumber[11];//�ֻ�����
char ConversionNum[44];//�ֻ�����ת���������

u8 SendFlag=0;						//��ͬ������־�����Ͳ�ͬ���ݶ���
u8 GpsInitOkFlag=0; 			//GPS��λ���ճɹ���־
u8 SOS_Flag=0;						//һ��SOS��־
u8 SetPhoneNumFlag=0;     //1�����óɹ���2����ʧ��
u8 FangDao=0;             //������־
u8 VibrateFlag=0;         //�����𶯱�־

GPS_INFO   GPS;  //GPS��Ϣ�ṹ��

extern unsigned char rev_start;
extern unsigned char rev_stop;
extern unsigned char gps_flag;
u8 GPS_rx_flag = 0;

void gsm_atcmd_send(char *at)//GSM ATָ��ͺ���
{
    unsigned short waittry;//��ʱ����
    do
    {
        gsm_rev_start = 0;//���տ�ʼ��־����
        gsm_rev_okflag = 0;//������ɱ�־����
        waittry = 0;//��ʱ��������
        uart_send(at,0xFF);//���ڷ�������
        while(waittry ++ < 3000)//����while��ʱ
        {
            if (gsm_rev_okflag == 1)//�ȴ�GSM����OK
            {
                return;//���س�ȥ
            }
            DelayMs(1);
        }
    }
    while(gsm_rev_okflag == 0);
}

void gsm_init(void)//gsm��ʼ��
{
    gsm_atcmd_send("AT\r\n");//�����õ�
    DelayMs(1000);
    gsm_atcmd_send("AT+CSCS=\"UCS2\"\r\n");//����Ϊunicode����
    DelayMs(1000);
    gsm_atcmd_send("AT+CMGF=1\r\n");//����Ϊ�ı�ģʽ
    DelayMs(1000);
    gsm_atcmd_send("AT+CNMI=2,1\r\n");//������ʱ��ʾ�����洢��ģ���ڴ�
	  DelayMs(1000);
	  gsm_atcmd_send("AT+CMGD=1,4\r\n");//�������
	  DelayMs(1000);
	  gsm_atcmd_send("AT+CSMP=17,0,2,25\r\n");//���ö��ű���Ϊ5���ӣ���������
	  DelayMs(1000);
}

/*
*number Ϊ�Է��ֻ���
*/
void gsm_send_msg(const char*number,char * content)
{
    u8 len;
    unsigned char  gsm_at_txbuf[60];//GSM AT�������
	
    memset(gsm_at_txbuf, 0, 60);//��������
    strncpy((char *)gsm_at_txbuf,"AT+CMGS=\"",9);//��AT+CMGS=\"�����Ƶ�gsm_at_txbuf
    memcpy(gsm_at_txbuf + 9, number, 44);//���ֻ����븴�Ƶ�AT+CMGS=\"֮��
    len = strlen((char *)gsm_at_txbuf);//��ȡgsm_at_txbuf�ַ�������
    gsm_at_txbuf[len] = '"'; // AT+CMGS=\"12345678901\"
    gsm_at_txbuf[len + 1] = '\r';
    gsm_at_txbuf[len + 2] = '\n';//gsm_at_txbuf���յĸ�ʽ"AT+CMGS=\"�ֻ�����\"\r\n"

    uart_send(gsm_at_txbuf,0xFF);//������Ҫ���ܶ��ŵ��ֻ�����
    DelayMs(1000);

    uart_send(content,0xFF);  //����������
    DelayMs(100);

    printf("%c",0x1a);     //���ͽ�������
    DelayMs(10);
}

void PhoneNumTranscoding(void)//�ֻ���ת��
{
	  u8 i=0;
		for(i=0;i<11;i++)//�������Ķ����ֻ��ű���ת�룬ǰ����Ҫ����003
		{
				ConversionNum[i*4+0] = '0';
			  ConversionNum[i*4+1] = '0';
			  ConversionNum[i*4+2] = '3';
			  ConversionNum[i*4+3] = PhoneNumber[i];
		}
}

void AddressTranscoding(char *str1,char *str2)//��γ������ת��
{
	  u8 i=0,len;
	  char *buf = str1;
	  len = strlen(buf);//��ȡ�ַ�������
	
		for(i=0; i < 3; i++)//С����ǰ3λ
		{
				if(buf[i] != ' ')
				{    
            *str2++ = '0';
					  *str2++ = '0';
					  *str2++ = '3';
					  *str2++ = buf[i];
				}
		}
		
		*str2++ = '0';*str2++ = '0';//С����
		*str2++ = '2';*str2++ = 'E';

		i++;
		
		for(;i < len-1; i++)//С�����6λ
		{
				*str2++ = '0';
				*str2++ = '0';
				*str2++ = '3';
				*str2++ = buf[i];
		}
		*str2 = '\0';
}

void KeySettings(void)//�������ú���
{
	  unsigned char keynum = 0;
	
	  keynum = KEY_Scan(0);//��ȡ����ֵ
}

void CheckNewMcu(void)  // ����Ƿ����µĵ�Ƭ�����ǵĻ���մ洢����������
{
	  u8 comper_str[6];
		
	  STMFLASH_Read(FLASH_SAVE_ADDR + 0x10,(u16*)comper_str,5);
	  comper_str[5] = '\0';
	  if(strstr((char *)comper_str,"STM32") == NULL)  //�µĵ�Ƭ��
		{
			 STMFLASH_Write(FLASH_SAVE_ADDR + 0x10,(u16*)"STM32",5); //д�롰STM32���������´�У��
			 DelayMs(50);
			 STMFLASH_Write(FLASH_SAVE_ADDR + 0x40,(u16*)"12345678910",11);//�����ʼ�ֻ���
			 DelayMs(50);
	  }
		STMFLASH_Read(FLASH_SAVE_ADDR + 0x40,(u16*)PhoneNumber,11); //�����ֻ���
		PhoneNumTranscoding();
		DelayMs(100);
}

void ContentHandle(void)//�������ݴ����������Ķ��ű��������ת��ΪUnicode��
{
		char TXbuf[400],databuf[50];             //�������ݻ�����
	  char tempbuf[11]; 
	
		memset(TXbuf,0, 400);   			//��ջ�����
		memset(databuf,0,50);         //��ջ�����
	

		gsm_send_msg(ConversionNum,TXbuf);//���Ͷ���
		SendFlag=0;
}


void SMS_Receive(void)//���Ž���
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
			  memset(STM32_RX1_BUF, 0, STM32_RX1BUFF_SIZE);//�������
				STM32_Rx1Counter = 0;
			
	
        SendFlag=3;//���Ͷ��ű�־
		}
	
		if(strstr(STM32_RX1_BUF,"517395ED963276D7")!=NULL)//���յ��رշ�������
		{
				if(VibrateFlag==0&&SOS_Flag==0)
				{
						BEEP = 1;
						DelayMs(200);
						BEEP = 0;
				}
			  memset(STM32_RX1_BUF, 0, STM32_RX1BUFF_SIZE);//�������
				STM32_Rx1Counter = 0;
			
			
        
		}
		

		
	  if(strstr(STM32_RX1_BUF,"+CMTI:")!=NULL)        //���յ�����  +CMTI: "SM",1
		{
			  presult1 = strstr(STM32_RX1_BUF,"+CMTI:");
			  if(strstr(presult1,"\"SM\",") != NULL ) 
				{				
					  presult2 = strstr(presult1,"\"SM\",");	// presult2="SM",1
					  DelayMs(20);	
					  memset(gsm_at_txbuf1, 0, 20);//��շ��ͻ�����
					  memset(SmsNum, 0, 3);//��ջ�����
						strncpy(gsm_at_txbuf1,"AT+CMGR=",8);//��AT+CMGR="�����Ƶ�gsm_at_txbuf
					  memcpy(SmsNum,presult2 + 5,2);
						memcpy(gsm_at_txbuf1 + 8, SmsNum, 2);//AT+CMGR=1
						len = strlen(gsm_at_txbuf1);//��ȡgsm_at_txbuf�ַ�������
						gsm_at_txbuf1[len] = '\r';
						gsm_at_txbuf1[len + 1] = '\n';//gsm_at_txbuf���յĸ�ʽ"AT+CMGR=XXX\r\n"
						uart_send(gsm_at_txbuf1,0xFF);
				
					  memset(STM32_RX1_BUF, 0, STM32_RX1BUFF_SIZE);//�������
						STM32_Rx1Counter = 0;
				}
		}
}

void SMS_Send(void)//���ŷ���
{ 
	  if(SendFlag!=0)//���ŷ��ͱ�־������0��˵��Ҫ������
		{
				ContentHandle();
		}
		if(SetPhoneNumFlag==1)
		{
				SetPhoneNumFlag = 0;
			  gsm_send_msg(ConversionNum,"624B673A53F778018BBE7F6E6210529FFF01");//���Ͷ��ţ��ֻ��������óɹ���
		}
		if(SetPhoneNumFlag==2)
		{
				SetPhoneNumFlag = 0;
			  gsm_send_msg(ConversionNum,"624B673A53F778018BBE7F6E59318D25FF018BF760A8518D4ED47EC668385BF9624B673A53F77801FF01");//���Ͷ��ţ��ֻ���������ʧ�ܣ�
		}
}

int main(void)
{
	char error_num = 0;
	u16 count;
	u16 alarm_delay=0;
	
	DelayInit();
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);	//����NVIC�жϷ���2:2λ��ռ���ȼ���2λ��Ӧ���ȼ�
	CheckNewMcu();//����Ƿ�Ϊ�µĵ�Ƭ��
	KEY_Init(); //������ʼ��
	GPS_rx_flag = 0;
	Usart1_Init(9600);
	USART3_Init(9600);

	GPS_rx_flag = 1;
  GSM_LED = 1;
	while(1)
	{
		  SMS_Receive();//���Ž���
		  KeySettings();
		  if(FangDao==1)
			{
					FD_LED=1;
				  if(VIBRATE==1)
					{
							DelayMs(100);//�˳������ź�
						  if(VIBRATE==1) 
							{
									if(VibrateFlag==0)
									{
											VibrateFlag=1;
										  BEEP = 1;//��������
											alarm_delay = 0;
										  SendFlag = 1;//���Ͷ��ű�־
									}
							}
					}
					if(VibrateFlag==1)
					{
							if(alarm_delay++ >= 3000)//�𶯱�����ʱʱ��
						 {
								 VibrateFlag = 0;
								 if(SOS_Flag==0)BEEP = 0;//�������ر�
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
			if (rev_stop == 1 && count++>=1000)   //���������һ��
			{
					if (GPS_RMC_Parse(STM32_RX3_BUF, &GPS)) //����GPRMC
					{
							error_num = 0;
							gps_flag = 0;
							rev_stop  = 0;
							GpsInitOkFlag=1;
						  GPS_LED = 1;//GPSָʾ������
					}
					else
					{
							if (error_num++ >= 3) //���������Ч����3��
							{
									error_num = 3;
									GpsInitOkFlag = 0;
                  GPS_LED = 0;//GPSָʾ��Ϩ��
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
