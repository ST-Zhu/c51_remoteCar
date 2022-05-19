#include "nRF24L01.h" 
 	
uchar TX_ADDRESS[TX_ADR_WIDTH]={0xB2,0x52,0xF4,0x09,0x71}; //���͵�ַ
uchar RX_ADDRESS[RX_ADR_WIDTH]={0xB1,0x51,0xF3,0x08,0x70}; //���յ�ַ
uchar rate = 0x0F;    //2Mbs, 0dB  ���������濪��

sfr ADC_CONTR=0xBC;
sfr ADC_RES=0xBD;
sfr ADC_LOW2=0xBE;
sfr P1ASF=0x9D;

#define ADC_POWER 0x80
#define ADC_FLAG 0x10
#define ADC_START 0x08
#define ADC_SPEEDLL 0x00
#define ADC_SPEEDL 0x20
#define ADC_SPEEDH 0x40
#define ADC_SPEEDHH 0x60

sbit LED=P2^6;									//�ź�С����
uchar rece_buf[3];							//���ͼĴ���
uchar tece_buf[2];							//���ռĴ���

sbit RS=P1^0;
sbit RW=P1^1;
sbit E=P1^2;

void delay(uint t)
{
	uint a,b;
	for(a=0;a<t;a++)
		for (b=0;b<255;b++);
}

void Delay_ADC(uint n)
{
	uint x;
	while(n--)
	{
		x=5000;
		while(x--);
	}
}

void InitNRF24L01()
{
	uint i;
	LED=0;
	while(NRF24L01_Check());                    //�ȴ���⵽NRF24L01������Ż�����ִ��
	NRF24L01_RT_Init(TX_PLOAD_WIDTH,TX_ADDRESS,TX_ADR_WIDTH,RX_ADDRESS,RX_ADR_WIDTH,rate);
	for(i=0;i<6;i++)
		{
			delay(500);	
			LED=~LED;
		}
	LED=0;
}

void InitADC()
{
	P1ASF=0x1c;
	P1=P1|0x1c;
	ADC_RES=0;
	ADC_CONTR=ADC_POWER|ADC_SPEEDLL;
	Delay_ADC(2);
}

void write_command(uchar x)			//дָ���
{
	RS=0;
	RW=0;
	E=0;
	P0=x;
	E=1;
	delay(25);
	E=0;
}

void write_date(uchar x)				//д���ݺ���
{
	RS=1;
	RW=0;
	E=0;
	P0=x;
	E=1;
	delay(25);
	E=0;
}

void InitLCD1602()			//��ʼ������
{
	write_command(0x38);
	write_command(0x0c);
	write_command(0x06);
	write_command(0x01);
	write_command(0x80);//��ʼ��LCD1602
}

void Init()
{
	InitNRF24L01();			//��ʼ��NRF24l01
	InitADC();					//��ʼ��ADC
//	InitLCD1602();			//��ʼ��LCD1602
}

uchar GetADCResult(uchar ch)
{
	ADC_CONTR=ADC_POWER|ADC_SPEEDLL|ch|ADC_START;
	_nop_();
	_nop_();
	_nop_();
	_nop_();
	while(!(ADC_CONTR&ADC_FLAG));
	ADC_CONTR&=~ADC_FLAG;
	
	return ADC_RES;
}

void TX()
{
	rece_buf[0]=GetADCResult(3);
	rece_buf[1]=GetADCResult(4);
	rece_buf[2]=GetADCResult(5);
	SEND_BUF(rece_buf);				//��������
}

void RX()
{
	if(NRF_IRQ==0)	 												//�������ģ����յ�����
	{
		if(NRF24L01_RxPacket(rece_buf)==0)		//�ж��Ƿ��ȡ����
		{
			LED=1;
			write_command(0x80);
			write_date(tece_buf[0]);
			write_command(0x80+0x40);
			write_date(tece_buf[1]);			//��һ����ʾ�����ٶȣ��ڶ�����ʾ�����ٶ�
		}
	}
	else
	{
		LED=0;
		InitLCD1602();
	}
}

void main()
{	
	Init();											//��ʼ��
	while(1)
	{
		TX();			//��������
//		RX();			//��������
	}
}
