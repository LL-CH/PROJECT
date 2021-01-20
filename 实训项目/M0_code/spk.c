#include <LPC11xx.h>
#include "spk.h"

void spk_init()
{
		LPC_SYSCON->SYSAHBCLKCTRL |= 1 << 16;
		LPC_IOCON->R_PIO1_1 = (LPC_IOCON->R_PIO1_1 & ~0x7) | 0x1;
		LPC_GPIO1->DIR |= 1 << 1;
}

void spk_on(int x)
{
		int i;
		while(x--)
		{
			LPC_GPIO1->DATA ^= 1 << 1;
			for(i = 0; i <500; i++);    //xÎªÒôÉ«
		}
}