#include <LPC11xx.h>
#include "spi.h"

#if 0
PIO0_6   时钟
PIO0_8   MISO ---- 主入从出
PIO0_9   MOSI  

PIO2_4   LCD 片选
PIO2_6   FLASH 片选
PIO2_7   RFID 片选 
#endif

void spi_init()
{
  //spi时钟，管脚配置	
	LPC_SYSCON->PRESETCTRL |=  1<<0;   //禁止reset SPI0总线上的外围设备 
	LPC_SYSCON->SYSAHBCLKCTRL |=  1<<11;  //使能SPI0的时钟 --->其实也没必要写这个，因为默认的就是使能的 
	LPC_SYSCON->SYSAHBCLKCTRL |= 1<<16;  //使能IO配置模块的时钟
	LPC_SYSCON->SSP0CLKDIV = 16;   // 48M /24  === 2m,   SPI0的时钟 
	
	LPC_IOCON->PIO0_8 = (LPC_IOCON->PIO0_8  & (~0x7)) | 0x1;   //在IOCON配置模块下选择功能为MISO的引脚
	LPC_IOCON->PIO0_9 = (LPC_IOCON->PIO0_9  & (~0x7)) | 0x1;   //在IOCON配置模块下选择功能为MOSI的引脚
	   
	LPC_IOCON->SCK_LOC  = (LPC_IOCON->SCK_LOC & (~0x3)) | 0x2;//配置时钟 来源于GPIO0_6
	LPC_IOCON->PIO0_6 = (LPC_IOCON->PIO0_6 & (~0x7)) | 0x2;//GPIO0_6输出时钟
	
	LPC_GPIO2->DIR  |= (1<<4 | 1<<6 | 1<<7);   //配置GPIO2_4 GPIO2_6 GPIO2_7为输出口
	LPC_GPIO2->DATA |= (1<<4 | 1<<6 | 1<<7);   //GPIO2_4 GPIO2_6 GPIO2_7输出高电平，不选通从SPI设备

	

  //SPI相关寄存器的配置
	LPC_SSP0->CR0 = (LPC_SSP0->CR0 & ~(0x15)) | 0x7;// 控制寄存器 --》数据位8个
	LPC_SSP0->CR1 = 0x2;//使能SPI的控制 ----》(在spi1这个可以操作的寄存器里面配置)
	LPC_SSP0->CPSR = 2; //时钟预分频寄存器 (偶数即可)
	
	
	
	return;
}

unsigned char spi_send(unsigned char  data)
{
	unsigned char chr;
	LPC_GPIO2->DATA &= (~(1<<7));   //选通RFID--》置低电平

	while(!(((LPC_SSP0->SR & (1<<4)) == 0) && (((LPC_SSP0->SR & (1<<1)) != 0)))) //SR为状态寄存器
	{
		;
	}
	
	LPC_SSP0->DR = data;  //第4位不忙，第2位以前为空，现在有数据
	
	
	while(!(((LPC_SSP0->SR & (1<<4)) == 0) && ((LPC_SSP0->SR & (1<<2)) != 0)))
	{
		;
	}
	chr = LPC_SSP0->DR;
	
	LPC_GPIO2->DATA |= 1<<7;   //置高电平，清除从设备，不选通RFID
	
	return chr;
	
}

