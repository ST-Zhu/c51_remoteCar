#include "nRF24L01.h" 
#include "DataScope_DP.h"
#define CYCLE 255	//PWM����
#define C 0.19  //�����ܳ�
#define T 0.1  //����
 	
uchar TX_ADDRESS[TX_ADR_WIDTH]={0xB1,0x51,0xF3,0x08,0x70}; //���͵�ַ		//�����շ���ַ
uchar RX_ADDRESS[RX_ADR_WIDTH]={0xB2,0x52,0xF4,0x09,0x71}; //���յ�ַ
uchar rate = 0x0F;    //2Mbs, 0dB  ���������濪��

sbit LED=P2^6;									//�źŵƽӿ�
uchar UD,LR,QS;									//С��״̬�ж���
uchar rece_buf[3];							//���ռĴ���
uchar tece_buf[2];							//���ͼĴ���

sbit ENA=P1^0;
sbit IN1=P1^1;
sbit IN2=P1^2;
sbit IN3=P1^3;
sbit IN4=P1^4;
sbit ENB=P1^5;
uchar count_R,count_L;		//������
uchar PWM_R,PWM_L;				//��ռ�ռ��

uchar temp_R,temp_L;				//��������
uchar time1,time2;				//�ӳ����ڷ�������ʱ��
uchar flag;								//��λ��

void delay(uint t)
{
	uint a,b;
	for(a=0;a<t;a++)
		for (b=0;b<255;b++);
}

void InitCar()
{
	ENA=0;
	IN1=0;
	IN2=0;
	IN3=0;
	IN4=0;
	ENB=0;
}

void InitNRF24L01()
{
	uint i;
	LED=0;
	while(NRF24L01_Check());         	//�ȴ���⵽NRF24L01������Ż�����ִ��
	NRF24L01_RT_Init(TX_PLOAD_WIDTH,TX_ADDRESS,TX_ADR_WIDTH,RX_ADDRESS,RX_ADR_WIDTH,rate);			
	for(i=0;i<6;i++)
		{
			delay(100);
			LED=~LED;
		}																//�Լ�ɹ�
	LED=0;
}

void InitTimer0()
{
	IP=0x02;		//���ö�ʱ��0���ȼ����
	TMOD|=0x01;
	TH0=(65536-10)/256;
	TL0=(65536-10)%256;
	EA=1;
	ET0=1;
	TR0=1;
}

void InitUart()
{
	TMOD|=0x20;
	TH1=0xfd;
	TL1=0xfd;	//������9600
	TR1=1;
	SM0=0;
	SM1=1;
}

void InitExter()
{
	EX0=1;
	IT0=1;
	EX1=1;
	IT1=1;
}

void Init()
{
	InitCar();						//��ʼ��PWMС��
	InitNRF24L01();				//��ʼ��NRF24L01
	InitTimer0();					//����ʱ��0
	InitUart();						//��ʼ������
//	InitExter();					//��ʼ���ⲿ�ж�
}

void RX()
{
	UD=rece_buf[0];
	LR=rece_buf[1];
	QS=rece_buf[2];
	LED=1;									//���ܳɹ�
}

void TO_speed()
{
	PWM_R=QS;
	PWM_L=QS;
}

void up()
{
	IN1=0;
	IN2=1;
	IN3=1;
	IN4=0;
}

void down()
{
	IN1=1;
	IN2=0;
	IN3=0;
	IN4=1;
}

void left()
{
	PWM_L/=5;
}

void right()
{
	PWM_R/=5;
}

void START()
{
	InitCar();									//��ʼ��С��
	TO_speed();										//�涨�ٶ�
	if(UD>150)  up();						//ǰ��
	if(UD<100)  down();					//����
	if(LR>150)  left();					//��ת
	if(LR<100)  right();				//��ת

}

void STOP()
{
	LED=0;
	ENA=0;
	IN1=0;
	IN2=0;
	IN3=0;
	IN4=0;
	ENB=0;
}

uchar NOW_speed_L()//�����ٶ�
{
	float V_L,n_L;
	n_L=temp_L/20.0;	//ת��
	V_L=n_L*C/T;	//С�������ٶ�
	temp_L=0;	//����������
	return V_L;
}

uchar NOW_speed_R()//�����ٶ�
{
	float V_R,n_R;
	n_R=temp_R/20.0;	//ת��
	V_R=n_R*C/T;	//С�������ٶ�
	temp_R=0;	//����������
	return V_R;
}

void send(float speed_L,float speed_R)	//�๦�ܴ�����ʾ����
{
	uchar i;
	uchar Send_Count;
	DataScope_Get_Channel_Data(speed_L,1);
	DataScope_Get_Channel_Data(speed_R,2);
	Send_Count=DataScope_Data_Generate(2);
	for(i=0;i<Send_Count;i++)
	{
		SBUF=DataScope_OutPut_Buffer[i];
		while(!TI);	
			TI=0;
	}
	temp_R=0;
	temp_L=0;	//����������
	delay(5);	//�ӳ�һ��
}

void TX()
{
	tece_buf[0]=NOW_speed_L();
	tece_buf[1]=NOW_speed_R();
	SEND_BUF(tece_buf);				//��������
}

void main()
{
	Init();
	while(1)
	{
		if(NRF_IRQ==0)	 												//�������ģ����յ�����
		{
			if(NRF24L01_RxPacket(rece_buf)==0)		//�ж��Ƿ��ȡ����
			{
				RX();							//�������ݲ���ʾ
				START();					//����С��
//				if(flag==1)
//				{
//					flag=0;
//					TX();				//����
//					send(NOW_speed_L(),NOW_speed_R());			//��ʾ����
//				}
			}
		}
		else
		{
			STOP();						//ֹͣ
		}
	}
}

void timer0() interrupt 1 //��ʱ��0�жϣ�10us��
{
	count_R++;
	count_L++;
	
	if(count_R<=PWM_R)
		ENA=1;
  else
		ENA=0;
	if(count_R==CYCLE)
	{
		ENA=~ENA;
		count_R=0;
	}
	
	if(count_L<=PWM_L)
		ENB=1;
  else
		ENB=0;
	if(count_L==CYCLE)
	{
		ENB=~ENB;
		count_L=0;
	}											//PWM����
	
	time1++;
	if(time1==100)
	{
		time1=0;
		time2++;
		if(time2==100)
		{
			flag=1;
			time2=0;
		}
	}			//100ms����һ������
	
	TH0=(65536-10)/256;
	TL0=(65536-10)%256;		//װ��ֵ
}

void exter0() interrupt 0	//С�����ֲ����ⲿ�ж�0
{
	temp_R++;
}

void exter1() interrupt 2	//С�����ֲ����ⲿ�ж�1
{
	temp_L++;
}
