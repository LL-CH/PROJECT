#include <stdio.h>
#include "string.h"
#include <LPC11xx.h>
#include "spi.h"
#include "rfid.h"

const unsigned char RFID_READ_DATA_BLOCK_21[10]  = {0x0a, 0x21, 0x00, 0x01, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};  //读指令
const unsigned char RFID_WRITE_DATA_BLOCK_22[10] = {0x1a, 0x22, 0x00, 0x01, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};  //写指令



/*
	计算checksum,计算校验和 （把要发送的数据全部加起来）
*/


//校验和
unsigned char rfid_checksum(const unsigned char *buf)
{
	
	int i;
	unsigned char chksum = 0;
	//把buf中的每一位数据相加得到一个数，作为校验保证数据的准确性
	for (i = 0; i < buf[0]; i++)   
	{
		chksum += buf[i];
	}
	return chksum;
}

/* rfid模块的读函数
rbuf: 读到的数据存放在rbuf里面
返回值：返回0----读成功
				返回-1-----读失败
*/

int rfid_read(unsigned char *rbuf)
{
	int i;
	unsigned char buf[32];
	unsigned char chksum = rfid_checksum(RFID_READ_DATA_BLOCK_21);    //得到读指令的校验和
	
  //要读rfid卡，遵守rfid模块的协议，先发送0xAA,0xBB,发送RFID_READ_DATA_BLOCK_21数组的内容过去
	//最后发送校验和checksum
	
	//0XAA, 0XBB是协议中用来收发数据时的确认字符，就像是对暗号一样，说正事之前先对暗号
	spi_send(0xAA);
	delay(10);
	spi_send(0xBB);
	delay(10);
	
	//先给rfid模块发一个读指令
	for (i = 0; i < RFID_READ_DATA_BLOCK_21[0]; i++)
	{
		spi_send(RFID_READ_DATA_BLOCK_21[i]);
		delay(10);
	}
	
	//校验和保证数据正确
	spi_send(chksum);
	
	//这个延时是等待rfid模块解析读指令和校验数据
	delay(500);
	
	
  //此时rfid知道你要读卡号了，此时你在发起确认字符
  //发送0xAA,发送0xBB，通过假写，读取rfid模块发给我们的数据，保存在数组buf里面
	spi_send(0xAA);
	delay(10);
	spi_send(0xBB);
	delay(10);
	
	
	//给rfid模块写19个0，这叫假写，是为了每写一个字符，spi_send函数的返回值会把rfid中我们要读的数据返回一位，如此往复，就得到了rfid模块的19个数据
	for (i = 0; i < 19; i++)
	{
		buf[i] = spi_send(0);
		delay(10);
	}
	
  /*根据rfid模块的协议，判断此次收到的数据是否有误，如果buf[0]==18 && buf[1]==0x21,说明数据正确，否则有问题,
		返回-1，如果数据正确，把读到的数据（buf里面存放的）拷贝到rbuf里面
	*/
	if(buf[0] != 18 || buf[1] != 0x21)
	{
		return -1;
	}
	else
	{
		//如果发送回来的数据经过校验没有问题的话，将buf中的后16字节（16字节卡号）拷贝到rbuf中就完成了读操作
		memcpy(rbuf, buf + 2, 16);		//memcpy与strcpy不同的是strcpy只能拷贝字符串，而memcpy什么都可以
		return 0;
	}
}


/* 
	rfid模块的写函数
	wbuf ---- 要写给rfid的数据
返回值：0-----写成功
				-1 ---- 写失败
*/


int rfid_write(unsigned char *wbuf)
{
	int i;
	unsigned char chksum;
	unsigned char buf[32];
  //写卡号的时候首先发送写指令然后是再是要写的16位卡号
	memcpy(buf, RFID_WRITE_DATA_BLOCK_22, 10);
	memcpy(buf + 10, wbuf, 16);
	chksum = rfid_checksum(buf);
  //写数据给rfid卡，先发送0xAA,发送0xBB,接下来发送buf里面的内容，26个字节，最后发送buf的checksum
  //当前buf里面存放的是数组RFID_WRITE_DATA_BLOCK_22 的内容和要写入的卡号也就是传过来的形参里面的数据，总共当前buf里面存放了26个字节
	
	//同样的通信前的确认（暗号）
	spi_send(0xAA);
	delay(10);
	spi_send(0xBB);
	delay(10);
	
	
	//将buf中的数据全部写过去
	for (i = 0; i < buf[0]; i++)
	{
		spi_send(buf[i]);
		delay(10);
	}
	//校验和
	spi_send(chksum);
	
	//等待解析
	delay(500);
	
  //发送0xAA,0xBB,假写3次，保存对方的返回值在buf里面
	spi_send(0xAA);
	delay(10);
	spi_send(0xBB);
	delay(10);
	
	
	//为了验证rfid是否写卡成功，我们要得到3位校验数据，所以进行3次假写
	for (i = 0; i < 3; i++)
	{
		buf[i] = spi_send(0);
		delay(10);
	}
	
  // 判断收到的数据，成功返回0，失败返回-1
	if (buf[0] != 2 || buf[1] != 0x22)
	{
		return -1;
	}
	else
	{
		return 0;
	}
}

