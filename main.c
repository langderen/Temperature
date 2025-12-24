#include <REGX52.H>
#include "Timer0.h"
#include "Key.h"
#include "Nixie.h"
#include "Delay.h"
#include "AT24C02.h"
#include "Buzzer.h"
#include "DS18B20.h"
#include "UART.h"
#include <stdio.h>
//波特率为9600
unsigned char KeyNum;
int Hour=0,Min=1,Sec=1,tmp;
unsigned char RunFlag,Buzz=0;
unsigned char index=0;
float T;
bit SendUARTFlag = 0;      // 1秒到时的发送标志
char UART_Str[30];         // 足够容纳 "99:59:59 --> 99.99\r\n"
void main()
{
	
	Timer0_Init();
	UART_Init();
	DS18B20_ConvertT();		//上电先转换一次温度，防止第一次读数据错误
	Delay(1000);			//等待转换完成
	
	while(1)
	{

		tmp=(int)(T*100);
		KeyNum=Key();

		if(KeyNum==1)			//K1按键按下用于启动或暂停计时过程
		{
			RunFlag=!RunFlag;	//启动标志位翻转
		}
		if(KeyNum==2)			//K2按键按下用于计时清零及系统复位
		{	
			RunFlag=0;
			Hour=0;					//分秒清0
			Min=1;				
			Sec=1;
			Buzz=0;					//暂停蜂鸣器
						
			AT24C02_WriteByte(0,tmp/100);//温度计整数部分
			AT24C02_WriteByte(1,tmp%100);

		}
		if(KeyNum==3)			//K3按键按下用于切换计时档位
		{
			if(++index>2)index=0;

		}
		if(KeyNum==4&&!RunFlag)			//K4按键按下用于对当前计时时间进行逐次增加，最小步长为1
		{
			if(index==0)Sec++;
			else if(index==1)Min++;
			else Hour++;
			
			if(Sec >= 60) Sec = 0;
			if(Min >= 60) Min = 0;
			if(Hour >= 99) Hour = 0;
		}
		Nixie_SetBuf(1,Hour/10);	//设置显示缓存，显示数据
		Nixie_SetBuf(2,Hour%10);
		Nixie_SetBuf(3,11);
		Nixie_SetBuf(4,Min/10);
		Nixie_SetBuf(5,Min%10);
		Nixie_SetBuf(6,11);
		Nixie_SetBuf(7,Sec/10);
		Nixie_SetBuf(8,Sec%10);
		
		
		// 每1秒发送一次 UART 数据
		
		if(SendUARTFlag) {		
			// 关中断保护 DS18B20 时序
      EA = 0; 
      DS18B20_ConvertT(); // 转换温度
      T = DS18B20_ReadT(); // 读取温度
      EA = 1; // 恢复中断
            
      SendUARTFlag = 0;  // 清标志
			sprintf(UART_Str, "%02d:%02d:%02d --> %d.%02dC\r\n",Hour, Min, Sec, tmp / 100, tmp % 100);
			ET0 = 0; // 临时关闭定时器0中断，停止按键扫描
			// ... 发送数据 ...
			UART_SendString(UART_Str);
			ET0 = 1; // 发送完毕，恢复中断
		}
	}
}

/**
  * @brief  秒表驱动函数，在中断中调用
  * @param  无
  * @retval 无
  */
void Sec_Loop(void)
{
	if(RunFlag==1)
	{
		Sec--;
		if(Sec<0)
		{
			Sec=59;
			Min--;
			if(Min<0)
			{
				Min=59;
				Hour--;
				if(Hour<0)
				{	Hour=Min=Sec=0;
					RunFlag=0;
					Buzz=1;
				}
			}
		}
	}
}

void Timer0_Routine() interrupt 1
{
	static unsigned int T0Count1,T0Count3;
	TL0 = 0x66;     
  TH0 = 0xFC;
	T0Count1++;
	if(T0Count1>=20)
	{
		T0Count1=0;
		Key_Loop();	//20ms调用一次按键驱动函数
	}

	Nixie_Loop();//2ms调用一次数码管驱动函数
	
	T0Count3++;
	if(T0Count3>=1000)
	{
		T0Count3=0;
		Sec_Loop();	//1s调用一次数秒表驱动函数
		SendUARTFlag=1;
	}

	if(Buzz)Buzzer_Time();
			
}
void UART_Routine() interrupt 4
{
	if(RI==1)					//如果接收标志位为1，接收到了数据
	{
		RI=0;					//接收标志位清0
	}
}