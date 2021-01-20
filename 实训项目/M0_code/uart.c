#include <LPC11xx.h>
#include "uart.h"

//uartÄ£¿éµÄ³õÊ¼»¯º¯Êý£¬ÓÃÀ´´ò¿ªIOCONÄ£¿éµÄÊ±ÖÓ£¬ÓÃÀ´ÅäÖÃ¼Ä´æÆ÷µÄ¹¦ÄÜ·Ö±ðÎªÊÕºÍ·¢
//Í¬Ê±´ò¿ªUARTÄ£¿éµÄÊ±ÖÓ£¬À´ÅäÖÃ²¨ÌØÂÊ
void uart_init()
{
	//´ò¿ªIOCONÄ£¿éµÄÊ±ÖÓ
	LPC_SYSCON->SYSAHBCLKCTRL |= (1<<16);
	//ÐÞ¸Ä¼Ä´æÆ÷µÄÖµ·Ö±ðÎª½ÓÊÕºÍ·¢ËÍµÄ¹¦ÄÜ
	LPC_IOCON->PIO1_6=(LPC_IOCON->PIO1_6 & ~0x7) | 0x1;//Ñ¡ÔñRXD
	LPC_IOCON->PIO1_7=(LPC_IOCON->PIO1_7 & ~0x7) | 0x1;//Ñ¡ÔñTXD
	
	
	//´ò¿ªUARTÄ£¿éµÄÊ±ÖÓ
	LPC_SYSCON->SYSAHBCLKCTRL |= 1<<12;
	//ÉèÖÃUARTÄ£¿éµÄ·ÖÆµÊý
	LPC_SYSCON->UARTCLKDIV = 4;
	//ÅäÖÃÏß¿ØÖÆ¼Ä´æÆ÷µÄÖµLCR
	LPC_UART->LCR = 0x83;
	
	//ÅäÖÃ²¨ÌØÂÊ
	
	//ÅäÖÃ³ýÊýËø´æ¼Ä´æÆ÷µÄÖµ--¡·DLLºÍDLM
	LPC_UART->DLL = 4; //µÍ°ËÎ»
	LPC_UART->DLM = 0; //¸ß°ËÎ» 
	//ÅäÖÃ·ÖÊý·ÖÆµ¼Ä´æÆ÷µÄÖµ-->FDR
	LPC_UART->FDR = 0x85;
	//¹Ø±ÕµôLCRÀïÃæµÄDLAB¼´³ýÊýËø´æ·ÃÎÊÎ»£¬½«¸ÃÎ»ÖÃ0¼´¿É
	LPC_UART->LCR=0x3; 
	
	//Ê¹ÄÜ·¢ËÍ¼Ä´æÆ÷Ò»Ö±´¦ÓÚ±£³Ö·¢ËÍµÄ×´Ì¬
	LPC_UART->THR |= 0x7;
	
}

//´®¿ÚµÄ½ÓÊÕ×Ö·ûµÄº¯Êý£¬uartÊÇÒ»¸öÒ»¸ö×Ö·û½øÐÐ²Ù×÷µÄ
char uart_getchar()
{
	//ÅÐ¶ÏLSRÖÐµÄµÚ0Î»µÄ×´Ì¬Îª0»¹ÊÇ1£   --¡·0´ú±íÃ»ÓÐÊý¾Ý£¬1´ú±í´ËÊ±ÒÑ¾­½ÓÊÕµ½Êý¾Ý
	while(!(LPC_UART->LSR & (1<<0)))
	{
		;
	}
	return LPC_UART->RBR;
}

void uart_putchar(char chr)
{
	//ÅÐ¶ÏLSRÖÐµÄµÚÎåÎ»µÄ×´Ì¬Îª0»¹ÊÇ1£¬Îª0´ú±í´ËÊ±ÓÐÎ´·¢ËÍÍêµÄÊý¾Ý£¬ÐèÒªµÈ´ý£¬Îª1Ôò´ú±í´ËÊ±·¢ËÍÇøÃ»ÓÐÊý¾Ý£¬´ËÊ±¿ÉÒÔ·¢ËÍÊý¾Ý
	while(!((LPC_UART->LSR & (1<<5)) != 0))
	{
		; //´ú±íµÈ´ý·¢ËÍÇø·¢ËÍÍêÊý¾Ý
	}
	LPC_UART->THR = chr;
}




