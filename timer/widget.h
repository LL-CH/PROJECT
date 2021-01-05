#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QTimer>
#include <QTime>

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
    void on_startBtn_clicked();
    void timeoutSlot();

    void on_stopBtn_clicked();

    void on_cleanBtn_clicked();

private:
    Ui::Widget *ui;
    QTimer *timer;
    int i;
    QVector<QString> paths;
};

#endif // WIDGET_H
