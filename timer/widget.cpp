#include "widget.h"
#include "ui_widget.h"

Widget::Widget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Widget)
{
    ui->setupUi(this);
    paths.append(":/new/prefix1/14e2bc137998d4f2efd51055d7a8c27c.gif");
    paths.append(":/new/prefix1/4.jpg");
    paths.append(":/new/prefix1/49.jpg");
    paths.append(":/new/prefix1/t014258a40b58b164c7.jpg");
    paths.append(":/new/prefix1/t015b38dfbf75c503ad.jpg");
    timer=new QTimer(this);
    i=0;
    ui->lineEdit->setText(QString::number(i)); //设置lineEdit数字
    //设置LcdNumber显示字符个数
    ui->lcdNumber->setDigitCount(8);
    //00:00:00 时分秒
    QTime time (0,0,0);
    QString str=time.toString("hh:mm:ss");
    ui->lcdNumber->display(str);
    connect(timer,SIGNAL(timeout()),this,SLOT(timeoutSlot()));
}

Widget::~Widget()
{
    delete ui;
}

void Widget::on_startBtn_clicked()
{
    timer->start(1000);//信号发送间隔时间
}

void Widget::timeoutSlot()
{
    i++;
    ui->lineEdit->setText(QString::number(i));
    QTime time(0,0,0);
    QTime t=time.addSecs(i);
    QString str=t.toString("hh:mm:ss");
    ui->lcdNumber->display(str);
    QPixmap pix;

    if(i>0 && (i%5)<6)
    {
        int j=i-1;
        j%=5;
        pix.load(paths[j]);
        ui->label->setScaledContents(true);
        ui->label->setPixmap(pix);
    }
}

void Widget::on_stopBtn_clicked()
{
    timer->stop();
}

void Widget::on_cleanBtn_clicked()
{
    i=0;
    ui->lineEdit->setText(QString::number(i));
    QTime time(0,0,0);
    QString str=time.toString("hh:mm:ss");
    ui->lcdNumber->display(str);
    timer->stop();
}
