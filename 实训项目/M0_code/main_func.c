#include <string.h>
#include <LPC11xx.h>
#include "led.h"
#include "uart.h"
#include "spk.h"
#include "spi.h"
#include "rfid.h"



//延时函数
void delay(int loop)
{
	int i = loop * 10000;
	for(;i>0;i--)
	{
		;
	}
}
//是一个主函数，用来控制每一个子模块
int main()
{
	
	//项目功能：刚开始让两个灯同时发光，然后通过串口的发送指令来控制灯的亮灭
	//经过分析：可以通过多路分支选择语句switch_case来实现不同指令去执行每一个指令所对应的动作
	
	//先通过两个初始化的函数来进行每个模块的基础配置
	//各模块初始化
	
	//定义两个字符数组来初始化两个卡号
	char num_card1[20]={"1234567890123456"};
	char num_card2[20]={"0000111122223333"};
	
	int i; //定义循环变量i
	uart_init();
	spi_init();

	
	//定义一个数组来存放从键盘输入的新的卡号以及获取到的卡号
	 char buf[20] = {0};
	//初始化完成之后让两个灯同时发光
	led_init();
	led_on();
	//初始化的时候让蜂鸣器发声代表复位成功
	spk_init();
	spk_on(500);
	//通过串口发送指令a之后让两个灯同时灭掉
	while(1)
	{
		char rec;
		rec = uart_getchar();//通过串口的接收端获得一个从键盘输入的字符
		if('q' == rec)  //输入q 退出switch
		{
			break;
		}
		switch(rec)
		{			
			//输入a-->led亮，输入b--->led灭，输入c蜂鸣器响
				
				case 'a':
						led_on();
					break;
				case 'b':
						led_off();
					break;
				case 'c':
						spk_on(500);
					break;
			
				case 'r':  //读卡 
					
						//读卡之前先将之间的数组中的内容清空
						memset(buf,0,sizeof(buf));
						rfid_read(buf);
						for(i=0;i<20;i++)
						{
							uart_putchar(buf[i]);
						}
					break;
					
				case 'w': //写卡
					
						//写卡之前先将数组中的内容进行清空
						memset(buf,0,sizeof(buf));
						//从键盘获得16位的卡号
						for(i=0;i<16;i++)
						{
							buf[i] = uart_getchar();
						}
						buf[i] = 0;
						//调用rfid模块的写函数进行写卡号

						rfid_write(buf);
					break;
					
					
			}
		}
	
	
	//进入刷卡模块的检测
	while(1)
	{
		//调用rfid的读卡函数，进行读卡号
		memset(buf,0,sizeof(buf));
		rfid_read(buf);
		//读完卡之后进行显示
		for(i=0;i<20;i++)
		{
			uart_putchar(buf[i]);
		}
		if(strcmp(num_card1,buf) == 0 || strcmp(num_card2,buf) == 0)
		{	//如果比较发现当前卡号与所默认的正确卡号一致则让两个灯进行流水闪灭一次即可
			led_blink();
		}
			//如果与正确的卡号匹配不上则让蜂鸣器发声
		else 
			spk_on(500);
	}
	
	return 0;
}