#include "nRF24L01.h" 
#include "DataScope_DP.h"
#define CYCLE 255	//PWM周期
#define C 0.19  //轮子周长
#define T 0.1  //周期
 	
uchar TX_ADDRESS[TX_ADR_WIDTH]={0xB1,0x51,0xF3,0x08,0x70}; //发送地址		//更改收发地址
uchar RX_ADDRESS[RX_ADR_WIDTH]={0xB2,0x52,0xF4,0x09,0x71}; //接收地址
uchar rate = 0x0F;    //2Mbs, 0dB  低噪声增益开启

sbit LED=P2^6;									//信号灯接口
uchar UD,LR,QS;									//小车状态判断数
uchar rece_buf[3];							//接收寄存器
uchar tece_buf[2];							//发送寄存器

sbit ENA=P1^0;
sbit IN1=P1^1;
sbit IN2=P1^2;
sbit IN3=P1^3;
sbit IN4=P1^4;
sbit ENB=P1^5;
uchar count_R,count_L;		//计数数
uchar PWM_R,PWM_L;				//所占空间比

uchar temp_R,temp_L;				//被挡次数
uchar time1,time2;				//加长串口发送数据时间
uchar flag;								//计位数

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
	while(NRF24L01_Check());         	//等待检测到NRF24L01，程序才会向下执行
	NRF24L01_RT_Init(TX_PLOAD_WIDTH,TX_ADDRESS,TX_ADR_WIDTH,RX_ADDRESS,RX_ADR_WIDTH,rate);			
	for(i=0;i<6;i++)
		{
			delay(100);
			LED=~LED;
		}																//自检成功
	LED=0;
}

void InitTimer0()
{
	IP=0x02;		//设置定时器0优先级最高
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
	TL1=0xfd;	//波特率9600
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
	InitCar();						//初始化PWM小车
	InitNRF24L01();				//初始化NRF24L01
	InitTimer0();					//开定时器0
	InitUart();						//初始化串口
//	InitExter();					//初始化外部中断
}

void RX()
{
	UD=rece_buf[0];
	LR=rece_buf[1];
	QS=rece_buf[2];
	LED=1;									//接受成功
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
	InitCar();									//初始化小车
	TO_speed();										//规定速度
	if(UD>150)  up();						//前进
	if(UD<100)  down();					//后退
	if(LR>150)  left();					//左转
	if(LR<100)  right();				//右转

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

uchar NOW_speed_L()//计算速度
{
	float V_L,n_L;
	n_L=temp_L/20.0;	//转速
	V_L=n_L*C/T;	//小车右轮速度
	temp_L=0;	//计数数清零
	return V_L;
}

uchar NOW_speed_R()//计算速度
{
	float V_R,n_R;
	n_R=temp_R/20.0;	//转速
	V_R=n_R*C/T;	//小车右轮速度
	temp_R=0;	//计数数清零
	return V_R;
}

void send(float speed_L,float speed_R)	//多功能串口显示波形
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
	temp_L=0;	//计数数清零
	delay(5);	//延迟一会
}

void TX()
{
	tece_buf[0]=NOW_speed_L();
	tece_buf[1]=NOW_speed_R();
	SEND_BUF(tece_buf);				//发送数据
}

void main()
{
	Init();
	while(1)
	{
		if(NRF_IRQ==0)	 												//如果无线模块接收到数据
		{
			if(NRF24L01_RxPacket(rece_buf)==0)		//判断是否读取数据
			{
				RX();							//接收数据并提示
				START();					//启动小车
//				if(flag==1)
//				{
//					flag=0;
//					TX();				//发送
//					send(NOW_speed_L(),NOW_speed_R());			//显示波形
//				}
			}
		}
		else
		{
			STOP();						//停止
		}
	}
}

void timer0() interrupt 1 //定时器0中断（10us）
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
	}											//PWM调速
	
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
	}			//100ms发送一次数据
	
	TH0=(65536-10)/256;
	TL0=(65536-10)%256;		//装初值
}

void exter0() interrupt 0	//小车左轮测速外部中断0
{
	temp_R++;
}

void exter1() interrupt 2	//小车右轮测速外部中断1
{
	temp_L++;
}
