#include <LPC11xx.h>
#include "uart.h"

//uart模块的初始化函数，用来打开IOCON模块的时钟，用来配置寄存器的功能分别为收和发
//同时打开UART模块的时钟，来配置波特率
void uart_init()
{
	//打开IOCON模块的时钟
	LPC_SYSCON->SYSAHBCLKCTRL |= (1<<16);
	//修改寄存器的值分别为接收和发送的功能
	LPC_IOCON->PIO1_6=(LPC_IOCON->PIO1_6 & ~0x7) | 0x1;//选择RXD
	LPC_IOCON->PIO1_7=(LPC_IOCON->PIO1_7 & ~0x7) | 0x1;//选择TXD
	
	
	//打开UART模块的时钟
	LPC_SYSCON->SYSAHBCLKCTRL |= 1<<12;
	//设置UART模块的分频数
	LPC_SYSCON->UARTCLKDIV = 4;
	//配置线控制寄存器的值LCR
	LPC_UART->LCR = 0x83;
	
	//配置波特率
	
	//配置除数锁存寄存器的值--》DLL和DLM
	LPC_UART->DLL = 4; //低八位
	LPC_UART->DLM = 0; //高八位 
	//配置分数分频寄存器的值-->FDR
	LPC_UART->FDR = 0x85;
	//关闭掉LCR里面的DLAB即除数锁存访问位，将该位置0即可
	LPC_UART->LCR=0x3; 
	
	//使能发送寄存器一直处于保持发送的状态
	LPC_UART->THR |= 0x7;
	
}

//串口的接收字符的函数，uart是一个一个字符进行操作的
char uart_getchar()
{
	//判断LSR中的第0位的状态为0还是1�   --》0代表没有数据，1代表此时已经接收到数据
	while(!(LPC_UART->LSR & (1<<0)))
	{
		;
	}
	return LPC_UART->RBR;
}

void uart_putchar(char chr)
{
	//判断LSR中的第五位的状态为0还是1，为0代表此时有未发送完的数据，需要等待，为1则代表此时发送区没有数据，此时可以发送数据
	while(!((LPC_UART->LSR & (1<<5)) != 0))
	{
		; //代表等待发送区发送完数据
	}
	LPC_UART->THR = chr;
}




