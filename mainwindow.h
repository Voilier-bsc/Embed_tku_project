#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#include <QLCDNumber>
#include <QMainWindow>
#include <QTimer>
#include <QLCDNumber>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
    QTimer* m_timer;

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:

    void PrintLCD();
    void on_pushButton_clicked();
    void on_pushButton_2_clicked();
    void on_pushButton_3_clicked();
    void on_pushButton_4_clicked();
    void on_pushButton_5_clicked();
    void on_pushButton_6_clicked();

private:
    Ui::MainWindow *ui;
};

extern float dist_obj;
extern int flag_heart_normal;
extern int front_vehicle_speed;
extern int ego_vehicle_speed;
extern float thres_dis;
extern int servo_flag;
extern int flag_motor;
#endif // MAINWINDOW_H
