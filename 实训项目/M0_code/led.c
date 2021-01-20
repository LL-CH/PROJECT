#include <LPC11xx.h>
#include "led.h"


void led_init()
{
	LPC_GPIO3->DIR |= 1<<0;
	LPC_GPIO3->DIR |= 1<<1;
}

void led_on() //i=0 / 1  点亮0 / 1 LED
{
	LPC_GPIO3->DATA &= ~ (1<<0);
	LPC_GPIO3->DATA &= ~ (1<<1);
}

void led_off() //关闭
{
	LPC_GPIO3->DATA |=(1<<0);
	LPC_GPIO3->DATA |=(1<<1);
}

void led_blink() //流水灯
{
	//流水灯的实现
		LPC_GPIO3->DATA &= ~ (1<<0); //点亮
		delay(1000);
		LPC_GPIO3->DATA |=(1<<0);
		LPC_GPIO3->DATA &= ~ (1<<1);
		delay(1000);
		LPC_GPIO3->DATA |=(1<<1);
}