#include <LPC11xx.h>
#include "spi.h"

#if 0
PIO0_6   ʱ��
PIO0_8   MISO ---- ����ӳ�
PIO0_9   MOSI  

PIO2_4   LCD Ƭѡ
PIO2_6   FLASH Ƭѡ
PIO2_7   RFID Ƭѡ 
#endif

void spi_init()
{
  //spiʱ�ӣ��ܽ�����	
	LPC_SYSCON->PRESETCTRL |=  1<<0;   //��ֹreset SPI0�����ϵ���Χ�豸 
	LPC_SYSCON->SYSAHBCLKCTRL |=  1<<11;  //ʹ��SPI0��ʱ�� --->��ʵҲû��Ҫд�������ΪĬ�ϵľ���ʹ�ܵ� 
	LPC_SYSCON->SYSAHBCLKCTRL |= 1<<16;  //ʹ��IO����ģ���ʱ��
	LPC_SYSCON->SSP0CLKDIV = 16;   // 48M /24  === 2m,   SPI0��ʱ�� 
	
	LPC_IOCON->PIO0_8 = (LPC_IOCON->PIO0_8  & (~0x7)) | 0x1;   //��IOCON����ģ����ѡ����ΪMISO������
	LPC_IOCON->PIO0_9 = (LPC_IOCON->PIO0_9  & (~0x7)) | 0x1;   //��IOCON����ģ����ѡ����ΪMOSI������
	   
	LPC_IOCON->SCK_LOC  = (LPC_IOCON->SCK_LOC & (~0x3)) | 0x2;//����ʱ�� ��Դ��GPIO0_6
	LPC_IOCON->PIO0_6 = (LPC_IOCON->PIO0_6 & (~0x7)) | 0x2;//GPIO0_6���ʱ��
	
	LPC_GPIO2->DIR  |= (1<<4 | 1<<6 | 1<<7);   //����GPIO2_4 GPIO2_6 GPIO2_7Ϊ�����
	LPC_GPIO2->DATA |= (1<<4 | 1<<6 | 1<<7);   //GPIO2_4 GPIO2_6 GPIO2_7����ߵ�ƽ����ѡͨ��SPI�豸

	

  //SPI��ؼĴ���������
	LPC_SSP0->CR0 = (LPC_SSP0->CR0 & ~(0x15)) | 0x7;// ���ƼĴ��� --������λ8��
	LPC_SSP0->CR1 = 0x2;//ʹ��SPI�Ŀ��� ----��(��spi1������Բ����ļĴ�����������)
	LPC_SSP0->CPSR = 2; //ʱ��Ԥ��Ƶ�Ĵ��� (ż������)
	
	
	
	return;
}

unsigned char spi_send(unsigned char  data)
{
	unsigned char chr;
	LPC_GPIO2->DATA &= (~(1<<7));   //ѡͨRFID--���õ͵�ƽ

	while(!(((LPC_SSP0->SR & (1<<4)) == 0) && (((LPC_SSP0->SR & (1<<1)) != 0)))) //SRΪ״̬�Ĵ���
	{
		;
	}
	
	LPC_SSP0->DR = data;  //��4λ��æ����2λ��ǰΪ�գ�����������
	
	
	while(!(((LPC_SSP0->SR & (1<<4)) == 0) && ((LPC_SSP0->SR & (1<<2)) != 0)))
	{
		;
	}
	chr = LPC_SSP0->DR;
	
	LPC_GPIO2->DATA |= 1<<7;   //�øߵ�ƽ��������豸����ѡͨRFID
	
	return chr;
	
}

