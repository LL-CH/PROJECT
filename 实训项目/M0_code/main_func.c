#include <string.h>
#include <LPC11xx.h>
#include "led.h"
#include "uart.h"
#include "spk.h"
#include "spi.h"
#include "rfid.h"



//��ʱ����
void delay(int loop)
{
	int i = loop * 10000;
	for(;i>0;i--)
	{
		;
	}
}
//��һ������������������ÿһ����ģ��
int main()
{
	
	//��Ŀ���ܣ��տ�ʼ��������ͬʱ���⣬Ȼ��ͨ�����ڵķ���ָ�������ƵƵ�����
	//��������������ͨ����·��֧ѡ�����switch_case��ʵ�ֲ�ָͬ��ȥִ��ÿһ��ָ������Ӧ�Ķ���
	
	//��ͨ��������ʼ���ĺ���������ÿ��ģ��Ļ�������
	//��ģ���ʼ��
	
	//���������ַ���������ʼ����������
	char num_card1[20]={"1234567890123456"};
	char num_card2[20]={"0000111122223333"};
	
	int i; //����ѭ������i
	uart_init();
	spi_init();

	
	//����һ����������ŴӼ���������µĿ����Լ���ȡ���Ŀ���
	 char buf[20] = {0};
	//��ʼ�����֮����������ͬʱ����
	led_init();
	led_on();
	//��ʼ����ʱ���÷�������������λ�ɹ�
	spk_init();
	spk_on(500);
	//ͨ�����ڷ���ָ��a֮����������ͬʱ���
	while(1)
	{
		char rec;
		rec = uart_getchar();//ͨ�����ڵĽ��ն˻��һ���Ӽ���������ַ�
		if('q' == rec)  //����q �˳�switch
		{
			break;
		}
		switch(rec)
		{			
			//����a-->led��������b--->led������c��������
				
				case 'a':
						led_on();
					break;
				case 'b':
						led_off();
					break;
				case 'c':
						spk_on(500);
					break;
			
				case 'r':  //���� 
					
						//����֮ǰ�Ƚ�֮��������е��������
						memset(buf,0,sizeof(buf));
						rfid_read(buf);
						for(i=0;i<20;i++)
						{
							uart_putchar(buf[i]);
						}
					break;
					
				case 'w': //д��
					
						//д��֮ǰ�Ƚ������е����ݽ������
						memset(buf,0,sizeof(buf));
						//�Ӽ��̻��16λ�Ŀ���
						for(i=0;i<16;i++)
						{
							buf[i] = uart_getchar();
						}
						buf[i] = 0;
						//����rfidģ���д��������д����

						rfid_write(buf);
					break;
					
					
			}
		}
	
	
	//����ˢ��ģ��ļ��
	while(1)
	{
		//����rfid�Ķ������������ж�����
		memset(buf,0,sizeof(buf));
		rfid_read(buf);
		//���꿨֮�������ʾ
		for(i=0;i<20;i++)
		{
			uart_putchar(buf[i]);
		}
		if(strcmp(num_card1,buf) == 0 || strcmp(num_card2,buf) == 0)
		{	//����ȽϷ��ֵ�ǰ��������Ĭ�ϵ���ȷ����һ�����������ƽ�����ˮ����һ�μ���
			led_blink();
		}
			//�������ȷ�Ŀ���ƥ�䲻�����÷���������
		else 
			spk_on(500);
	}
	
	return 0;
}