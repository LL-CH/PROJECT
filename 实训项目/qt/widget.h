#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QtSerialPort>
#include <QSerialportInfo>

namespace Ui {
class Widget;
}

class Widget : public QWidget
{
    Q_OBJECT

public:
    explicit Widget(QWidget *parent = 0);
    ~Widget();

private slots:
   // void on_pushButton_2_clicked();

   // void on_pushButton_clicked();

    void on_handBtn_clicked();

   // void on_pushButton_4_clicked();

    void on_OpenpushButton_clicked();

    void on_cleSendBtn_clicked();

    void on_cleRecBtn_clicked();

    void readDateSlot();

    //void on_textBrowser_destroyed();

private:
    Ui::Widget *ui;
    QSerialPort *serialPort;
    QByteArray  arr;
};

#endif // WIDGET_H
