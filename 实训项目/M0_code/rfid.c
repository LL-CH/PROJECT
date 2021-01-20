#include <stdio.h>
#include "string.h"
#include <LPC11xx.h>
#include "spi.h"
#include "rfid.h"

const unsigned char RFID_READ_DATA_BLOCK_21[10]  = {0x0a, 0x21, 0x00, 0x01, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};  //��ָ��
const unsigned char RFID_WRITE_DATA_BLOCK_22[10] = {0x1a, 0x22, 0x00, 0x01, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};  //дָ��



/*
	����checksum,����У��� ����Ҫ���͵�����ȫ����������
*/


//У���
unsigned char rfid_checksum(const unsigned char *buf)
{
	
	int i;
	unsigned char chksum = 0;
	//��buf�е�ÿһλ������ӵõ�һ��������ΪУ�鱣֤���ݵ�׼ȷ��
	for (i = 0; i < buf[0]; i++)   
	{
		chksum += buf[i];
	}
	return chksum;
}

/* rfidģ��Ķ�����
rbuf: ���������ݴ����rbuf����
����ֵ������0----���ɹ�
				����-1-----��ʧ��
*/

int rfid_read(unsigned char *rbuf)
{
	int i;
	unsigned char buf[32];
	unsigned char chksum = rfid_checksum(RFID_READ_DATA_BLOCK_21);    //�õ���ָ���У���
	
  //Ҫ��rfid��������rfidģ���Э�飬�ȷ���0xAA,0xBB,����RFID_READ_DATA_BLOCK_21��������ݹ�ȥ
	//�����У���checksum
	
	//0XAA, 0XBB��Э���������շ�����ʱ��ȷ���ַ��������Ƕ԰���һ����˵����֮ǰ�ȶ԰���
	spi_send(0xAA);
	delay(10);
	spi_send(0xBB);
	delay(10);
	
	//�ȸ�rfidģ�鷢һ����ָ��
	for (i = 0; i < RFID_READ_DATA_BLOCK_21[0]; i++)
	{
		spi_send(RFID_READ_DATA_BLOCK_21[i]);
		delay(10);
	}
	
	//У��ͱ�֤������ȷ
	spi_send(chksum);
	
	//�����ʱ�ǵȴ�rfidģ�������ָ���У������
	delay(500);
	
	
  //��ʱrfid֪����Ҫ�������ˣ���ʱ���ڷ���ȷ���ַ�
  //����0xAA,����0xBB��ͨ����д����ȡrfidģ�鷢�����ǵ����ݣ�����������buf����
	spi_send(0xAA);
	delay(10);
	spi_send(0xBB);
	delay(10);
	
	
	//��rfidģ��д19��0����м�д����Ϊ��ÿдһ���ַ���spi_send�����ķ���ֵ���rfid������Ҫ�������ݷ���һλ������������͵õ���rfidģ���19������
	for (i = 0; i < 19; i++)
	{
		buf[i] = spi_send(0);
		delay(10);
	}
	
  /*����rfidģ���Э�飬�жϴ˴��յ��������Ƿ��������buf[0]==18 && buf[1]==0x21,˵��������ȷ������������,
		����-1�����������ȷ���Ѷ��������ݣ�buf�����ŵģ�������rbuf����
	*/
	if(buf[0] != 18 || buf[1] != 0x21)
	{
		return -1;
	}
	else
	{
		//������ͻ��������ݾ���У��û������Ļ�����buf�еĺ�16�ֽڣ�16�ֽڿ��ţ�������rbuf�о�����˶�����
		memcpy(rbuf, buf + 2, 16);		//memcpy��strcpy��ͬ����strcpyֻ�ܿ����ַ�������memcpyʲô������
		return 0;
	}
}


/* 
	rfidģ���д����
	wbuf ---- Ҫд��rfid������
����ֵ��0-----д�ɹ�
				-1 ---- дʧ��
*/


int rfid_write(unsigned char *wbuf)
{
	int i;
	unsigned char chksum;
	unsigned char buf[32];
  //д���ŵ�ʱ�����ȷ���дָ��Ȼ��������Ҫд��16λ����
	memcpy(buf, RFID_WRITE_DATA_BLOCK_22, 10);
	memcpy(buf + 10, wbuf, 16);
	chksum = rfid_checksum(buf);
  //д���ݸ�rfid�����ȷ���0xAA,����0xBB,����������buf��������ݣ�26���ֽڣ������buf��checksum
  //��ǰbuf�����ŵ�������RFID_WRITE_DATA_BLOCK_22 �����ݺ�Ҫд��Ŀ���Ҳ���Ǵ��������β���������ݣ��ܹ���ǰbuf��������26���ֽ�
	
	//ͬ����ͨ��ǰ��ȷ�ϣ����ţ�
	spi_send(0xAA);
	delay(10);
	spi_send(0xBB);
	delay(10);
	
	
	//��buf�е�����ȫ��д��ȥ
	for (i = 0; i < buf[0]; i++)
	{
		spi_send(buf[i]);
		delay(10);
	}
	//У���
	spi_send(chksum);
	
	//�ȴ�����
	delay(500);
	
  //����0xAA,0xBB,��д3�Σ�����Է��ķ���ֵ��buf����
	spi_send(0xAA);
	delay(10);
	spi_send(0xBB);
	delay(10);
	
	
	//Ϊ����֤rfid�Ƿ�д���ɹ�������Ҫ�õ�3λУ�����ݣ����Խ���3�μ�д
	for (i = 0; i < 3; i++)
	{
		buf[i] = spi_send(0);
		delay(10);
	}
	
  // �ж��յ������ݣ��ɹ�����0��ʧ�ܷ���-1
	if (buf[0] != 2 || buf[1] != 0x22)
	{
		return -1;
	}
	else
	{
		return 0;
	}
}

