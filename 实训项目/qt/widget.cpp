#include "widget.h"
#include "ui_widget.h"

Widget::Widget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Widget)
{
    ui->setupUi(this);
    //设置串口的标题为“串口助手”
    this->setWhatsThis("串口助手");
    //查找可用的串口
    foreach (const QSerialPortInfo &info,QSerialPortInfo::availablePorts())
    {
        QSerialPort serial;
        serial.setPort(info);
        if(serial.open(QIODevice::ReadWrite))
        {
            ui->COMcomboBox->addItem(serial.portName());
            serial.close();
        }
    }
    ui->handBtn->setEnabled(false);
}

Widget::~Widget()
{
    delete ui;
}

//清空发送区
void Widget::on_cleSendBtn_clicked()
{
    ui->textEdit->clear();
}
//清空接收区
void Widget::on_cleRecBtn_clicked()
{
    ui->textBrowser->clear();
}


void Widget::on_handBtn_clicked()//手动发送
{
    QString str = ui->textEdit->toPlainText();
    serialPort->write(str.toLatin1());
}




void Widget::on_OpenpushButton_clicked() //打开关闭串口
{
    if("打开串口" == ui->OpenpushButton->text())
    {
       QString str1 = ui->COMcomboBox->currentText();
       QString str2 = ui->BaudcomboBox->currentText();

       serialPort = new QSerialPort(this);
       serialPort->setPortName(str1);
       if(serialPort->open(QIODevice::ReadWrite))
       {
          serialPort->setBaudRate(str2.toInt()) ;//设置波特率
          serialPort->setParity(QSerialPort::NoParity);//设置校验位
          serialPort->setDataBits(QSerialPort::Data8);//设置数据位
          serialPort->setStopBits(QSerialPort::OneStop);//设置停止位
          //关闭摄制菜单使能
          ui->COMcomboBox->setEnabled(false);
          ui->BaudcomboBox->setEnabled(false);
          ui->DatecomboBox->setEnabled(false);
          ui->StopcomboBox->setEnabled(false);
          ui->ParitycomboBox->setEnabled(false);
          connect(serialPort,SIGNAL(readyRead()),this,SLOT(readDateSlot()));
          ui->handBtn->setEnabled(true);
       }
       else
       {
           //关闭串口
           serialPort->clear();
           serialPort->close();
           serialPort->deleteLater();
           //打开菜单使能
           ui->COMcomboBox->setEnabled(true);
           ui->BaudcomboBox->setEnabled(true);
           ui->DatecomboBox->setEnabled(true);
           ui->StopcomboBox->setEnabled(true);
           ui->ParitycomboBox->setEnabled(true);
           ui->OpenpushButton->setText("打开串口");
       }
    }
}
//读数据
void Widget::readDateSlot()
{
    QByteArray buf;
    buf = serialPort->readAll();
    if(!buf.isEmpty())
    {
        QString str = ui->textBrowser->toPlainText();
        str += QString(buf);
        ui->textBrowser->clear();
        ui->textBrowser->append(str);
    }
    buf.clear();
}




