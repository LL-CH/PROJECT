#include <LPC11xx.h>
#include "led.h"


void led_init()
{
	LPC_GPIO3->DIR |= 1<<0;
	LPC_GPIO3->DIR |= 1<<1;
}

void led_on() //i=0 / 1  ����0 / 1 LED
{
	LPC_GPIO3->DATA &= ~ (1<<0);
	LPC_GPIO3->DATA &= ~ (1<<1);
}

void led_off() //�ر�
{
	LPC_GPIO3->DATA |=(1<<0);
	LPC_GPIO3->DATA |=(1<<1);
}

void led_blink() //��ˮ��
{
	//��ˮ�Ƶ�ʵ��
		LPC_GPIO3->DATA &= ~ (1<<0); //����
		delay(1000);
		LPC_GPIO3->DATA |=(1<<0);
		LPC_GPIO3->DATA &= ~ (1<<1);
		delay(1000);
		LPC_GPIO3->DATA |=(1<<1);
}