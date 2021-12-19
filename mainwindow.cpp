#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <unistd.h>
#include <QtGui>
#include <math.h>
#include <QString>
#include <QColor>

float display_dist;
float display_front_vehicle_speed;
float display_ego_vehicle_speed;
float display_thres_dis;
QString danger;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    QTimer *timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(PrintLCD()));
    timer->start(500);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_pushButton_clicked()
{
    flag_heart_normal = 0;
}

void MainWindow::on_pushButton_2_clicked()
{
    flag_heart_normal = 1;
    servo_flag = 1;
    flag_motor = 0;
}

void MainWindow::PrintLCD()
{

    display_dist = floor(3*dist_obj * 100) / 100;
    QString str1 = QString::number(display_dist);
    QString str2 = QString::number(flag_heart_normal);
    display_front_vehicle_speed = floor(3.6*front_vehicle_speed * 10) / 10;
    QString str3 = QString::number(display_front_vehicle_speed);
    display_thres_dis = floor(3*thres_dis * 100) / 100;
    QString str4 = QString::number(display_thres_dis);
    display_ego_vehicle_speed = floor(3.6*ego_vehicle_speed * 10) / 10;
    QString str5 = QString::number(display_ego_vehicle_speed);

    if(flag_heart_normal == 0){
        danger = "dangerous!!!";
        ui->text_danger->setTextColor(QColor(255,0,0));
        ui->text_danger->setFontPointSize(30);

        ui->text_danger->setText(danger);
        ui->text_danger->setAlignment(Qt::AlignCenter);

    }
    else{
        danger = "normal!!!";
        ui->text_danger->setTextColor(QColor(0,0,0));
        ui->text_danger->setFontPointSize(30);

        ui->text_danger->setText(danger);
        ui->text_danger->setAlignment(Qt::AlignCenter);
    }
    ui->LCD_1->display(str1);
    ui->LCD_2->display(str2);
    ui->LCD_3->display(str3);
    ui->LCD_4->display(str4);
    ui->LCD_5->display(str5);

}


void MainWindow::on_pushButton_3_clicked()
{
    front_vehicle_speed = front_vehicle_speed + 1;
}

void MainWindow::on_pushButton_4_clicked()
{
    front_vehicle_speed = front_vehicle_speed - 1;
}

void MainWindow::on_pushButton_5_clicked()
{
    ego_vehicle_speed = ego_vehicle_speed + 1;
}

void MainWindow::on_pushButton_6_clicked()
{
    ego_vehicle_speed = ego_vehicle_speed - 1;
}
