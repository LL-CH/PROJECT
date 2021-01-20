#include <LPC11xx.h>
#include "uart.h"

//uartģ��ĳ�ʼ��������������IOCONģ���ʱ�ӣ��������üĴ����Ĺ��ֱܷ�Ϊ�պͷ�
//ͬʱ��UARTģ���ʱ�ӣ������ò�����
void uart_init()
{
	//��IOCONģ���ʱ��
	LPC_SYSCON->SYSAHBCLKCTRL |= (1<<16);
	//�޸ļĴ�����ֵ�ֱ�Ϊ���պͷ��͵Ĺ���
	LPC_IOCON->PIO1_6=(LPC_IOCON->PIO1_6 & ~0x7) | 0x1;//ѡ��RXD
	LPC_IOCON->PIO1_7=(LPC_IOCON->PIO1_7 & ~0x7) | 0x1;//ѡ��TXD
	
	
	//��UARTģ���ʱ��
	LPC_SYSCON->SYSAHBCLKCTRL |= 1<<12;
	//����UARTģ��ķ�Ƶ��
	LPC_SYSCON->UARTCLKDIV = 4;
	//�����߿��ƼĴ�����ֵLCR
	LPC_UART->LCR = 0x83;
	
	//���ò�����
	
	//���ó�������Ĵ�����ֵ--��DLL��DLM
	LPC_UART->DLL = 4; //�Ͱ�λ
	LPC_UART->DLM = 0; //�߰�λ 
	//���÷�����Ƶ�Ĵ�����ֵ-->FDR
	LPC_UART->FDR = 0x85;
	//�رյ�LCR�����DLAB�������������λ������λ��0����
	LPC_UART->LCR=0x3; 
	
	//ʹ�ܷ��ͼĴ���һֱ���ڱ��ַ��͵�״̬
	LPC_UART->THR |= 0x7;
	
}

//���ڵĽ����ַ��ĺ�����uart��һ��һ���ַ����в�����
char uart_getchar()
{
	//�ж�LSR�еĵ�0λ��״̬Ϊ0����1�   --��0����û�����ݣ�1�����ʱ�Ѿ����յ�����
	while(!(LPC_UART->LSR & (1<<0)))
	{
		;
	}
	return LPC_UART->RBR;
}

void uart_putchar(char chr)
{
	//�ж�LSR�еĵ���λ��״̬Ϊ0����1��Ϊ0�����ʱ��δ����������ݣ���Ҫ�ȴ���Ϊ1������ʱ������û�����ݣ���ʱ���Է�������
	while(!((LPC_UART->LSR & (1<<5)) != 0))
	{
		; //����ȴ�����������������
	}
	LPC_UART->THR = chr;
}




