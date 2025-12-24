#include <REGX52.H>

/**
  * @brief  串口初始化，9600bps@12.000MHz
  * @param  无
  * @retval 无
  */
void UART_Init()
{		
		SCON = 0x50;
    PCON &= 0x7F;   // 【重要修改】波特率不加倍 (SMOD=0)
    TMOD &= 0x0F;
    TMOD |= 0x20;
    TL1 = 0xFD;     // 11.0592MHz下，0xFD对应9600bps
    TH1 = 0xFD;
    ET1 = 0;
    TR1 = 1;
}

/**
  * @brief  串口发送一个字节数据
  * @param  Byte 要发送的一个字节数据
  * @retval 无
  */
void UART_SendByte(unsigned char Byte)
{
	SBUF=Byte;
	while(!TI);
	TI=0;
}

void UART_SendString(char *str)
{
    while (*str)
    {
        UART_SendByte(*str++);
    }
}