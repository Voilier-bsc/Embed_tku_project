#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <linux/input.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>
#include <linux/i2c-dev.h>
#include <stdbool.h>
#include "mainwindow.h"
#include <QApplication>
#include <math.h>
#define GPIO(BANK, IO) (BANK-1)*32+IO
#define SYSFS_GPIO_DIR "/sys/class/gpio"
#define MAX_BUF 64

#define SERVO1 GPIO(1, 26)
#define ULTRA_TRIGGER GPIO(1, 27)
#define ULTRA_ECHO GPIO(1, 28)
#define MOTOR_A GPIO(5, 21) //149
#define MOTOR_B GPIO(5, 20) //148

#define LED_BACK GPIO(1, 21)
#define LED_HOME GPIO(1, 16)
#define LED_MENU GPIO(1, 20)

#define RTC_RD_TIME _IOR('p', 0x09, struct linux_rtc_time)
#define IO_BASE_ADDR 0x20E0000
#define IO_BASE_ADDR_PWM_CONT 0x2088000
#define IO_BASE_ADDR_PWM_COUNTER 0X2088014
#define IO_BASE_ADDR_CCM 0x20C4000
#define U32 unsigned int
#define U16 unsigned short
#define IO_SIZE 0xff

#define GPIO_DATA_ADDR 0x209C000
#define GPIO_DIR_ADDR 0x209C004
#define GPIO_INT_MASK_ADDR 0x209C018

#define IOCTL_START_BUZZER _IOW('b', 0x07, int)
#define IOCTL_END_BUZZER _IOW('b', 0x09, int)
#define IOCTL_SET_TONE _IOW('b', 0x0b, int)
#define IOCTL_SET_VOLUME _IOW('b', 0x0c, int)
#define IOCTL_GET_TONE _IOW('b', 0x0d, int)
#define IOCTL_GET_VOLUME _IOW('b', 0x0e, int)

float dist_obj=20;
float dist_obj_prev=20;
int flag_heart_normal = 1;
int flag_calc  = 0;
int flag_motor = 0;
int front_vehicle_speed = 20;
int ego_vehicle_speed = 20;
float thres_dis;
int alpha = 0.9;
int servo_flag = 0;

int gpio_export(unsigned int gpio);
int gpio_unexport(unsigned int gpio);
int gpio_set_dir(unsigned int gpio, int value);
int gpio_set_value(unsigned int gpio, int value);
int gpio_read_value(unsigned int gpio);
long freqToTone(double freq);
void playTone(long tone, int volume, int time);
void Servo(unsigned int Servo, signed int angle);

long freqToTone(double freq) {
    double tone;
    tone = (1.0f / freq) * 1000000000;
    return (long) tone;
}

void playTone(long tone, int volume, int time) {
    int buzzer_fd = open("/dev/buzzer", O_RDONLY);

    ioctl(buzzer_fd, IOCTL_SET_VOLUME, volume);
    ioctl(buzzer_fd, IOCTL_SET_TONE, tone);
    ioctl(buzzer_fd, IOCTL_START_BUZZER, 0);
    usleep(time);
    ioctl(buzzer_fd, IOCTL_END_BUZZER, 0);
    close(buzzer_fd);
}

int gpio_export(unsigned int gpio){
    int fd, len;
    char buf[MAX_BUF];

    fd = open(SYSFS_GPIO_DIR "/export", O_WRONLY);

    if (fd < 0) {
        printf("gpio/export\n");
        return 1;
    }

    len = snprintf(buf, sizeof(buf), "%d", gpio);
    write(fd, buf, len);
    close(fd);

    return 0;
}

int gpio_unexport(unsigned int gpio){
    int fd, len;
    char buf[MAX_BUF];

    fd = open(SYSFS_GPIO_DIR "/unexport", O_WRONLY);

    if (fd < 0) {
        printf("gpio/unexport\n");
        return 1;
    }

    len = snprintf(buf, sizeof(buf), "%d", gpio);
    write(fd, buf, len);
    close(fd);

    return 0;
}

int gpio_set_dir(unsigned int gpio, int dir) {
    int fd;
    char buf[MAX_BUF];

    snprintf(buf, sizeof(buf), SYSFS_GPIO_DIR "/gpio%d/direction", gpio);
    fd = open(buf, O_WRONLY);

    if (fd < 0) {
        printf("gpio/direction\n");
        return 1;
    }

    if (dir)
        write(fd, "out\n", 4);
    else

    write(fd, "in\n", 3);
    close(fd);

    return 0;
}

int gpio_set_value(unsigned int gpio, int value) {
    int fd;
    char buf[MAX_BUF];

    snprintf(buf, sizeof(buf), SYSFS_GPIO_DIR "/gpio%d/value", gpio);
    fd = open(buf, O_WRONLY);

    if (fd < 0) {
        printf("gpio/set-value\n");
        return 1;
    }

    if (value == 1) {
        write(fd, "1\n", 2);
    }

    else {
        write(fd, "0\n", 2);
    }

    close(fd);
    return 0;
}

int gpio_read_value(unsigned int gpio) {
    int fd;
    int n;
    char buf[MAX_BUF];
    char out[MAX_BUF];
    snprintf(buf, sizeof(buf), SYSFS_GPIO_DIR "/gpio%d/value", gpio);
    fd = open(buf, O_RDONLY);
    if (fd < 0) {
        printf("gpio/read-value\n");
        return 1;
    }

    read(fd, out, MAX_BUF);
    n = out[0]-48;
    close(fd);
    return n;
}

void *Calculate_Dist(void *arg){
    unsigned int trig = ULTRA_TRIGGER;
    unsigned int echo = ULTRA_ECHO;

    gpio_export(trig);
    gpio_export(echo);
    gpio_set_dir(trig, 1);
    gpio_set_dir(echo, 0);
    float duration;
    float dist_obj_now;
    clock_t start1, end1;
    while(1){
        if (flag_calc==0){
            flag_calc = 1;
            int trig_flag = 0;
            gpio_set_value(trig, 1);
            usleep(10);
            gpio_set_value(trig, 0);

            while(1){

                if((gpio_read_value(echo) == 1) && (trig_flag == 0)){
                    start1 = clock();
                    trig_flag = 1;
                }
                if((gpio_read_value(echo) == 0) && (trig_flag == 1)){
                    trig_flag = 0;
                    end1 = clock();
                    break;
                }
            }

            duration = (float)(end1 - start1)/CLOCKS_PER_SEC;
            dist_obj_now = duration * 17 * 1000; // [cm]
            usleep(1000000);
            flag_calc = 0;
            dist_obj = alpha*dist_obj_prev + (1-alpha)*dist_obj_now;
            dist_obj_prev = dist_obj;
        }
    }
}

void *MotorDrive(void *arg){
    unsigned int motor_a = MOTOR_A;
    unsigned int motor_b = MOTOR_B;

    gpio_export(motor_a);
    gpio_export(motor_b);

    gpio_set_dir(motor_a, 1);
    gpio_set_dir(motor_b, 1);

    gpio_set_value(motor_a, 1);
    gpio_set_value(motor_b, 0);
    unsigned int a;

    while(1){
        int Value = 800 + (ego_vehicle_speed-20)*40;
        if (flag_motor == 0){
            if((flag_heart_normal == 0) && (dist_obj>thres_dis)){

                gpio_set_value(LED_HOME, 1);

                for(a=Value; a>-1; a--)
                {
                    gpio_set_value(motor_a, 1);
                    usleep(a);
                    gpio_set_value(motor_a, 0);
                }
                servo_flag = 0;
                flag_motor = 1;
            }
            else{

                gpio_set_value(LED_HOME, 0);
                for(a=0; a<5; a++)
                {
                    gpio_set_value(motor_a, 1);
                    usleep(Value);
                    gpio_set_value(motor_a, 0);
                }
            }
            usleep(200);
        }
    }
}

void *Servo_cont(void *arg){
    unsigned int servo = SERVO1;
    int i;
    long freq = 880;


    while(1){
        thres_dis = ((front_vehicle_speed - ego_vehicle_speed)*1.6 + pow(ego_vehicle_speed,2)/(2*0.8*9.81))/3;

        if((flag_heart_normal == 0)&&(dist_obj<thres_dis)&&(servo_flag == 1)){
            gpio_set_value(LED_HOME, 0);
            // buzzer 삽입
            for (i=0; i<50; i=i+5){
                Servo(servo, -i);
                playTone(freq, 25000, 100 * 1000);
            }
            for (i=50; i>-1; i=i-5){
                Servo(servo, -i);
                playTone(freq, 25000, 100 * 1000);
            }
            usleep(100000);
        }
    }
}

void Servo(unsigned int Servo, signed int angle)
{
    unsigned int value;
    unsigned int a;

    /* Set PWM */
    gpio_export(Servo);
    gpio_set_dir(Servo, 1);

    value = 1400 + angle*10;
    for(a=0; a<5; a++)
    {
        gpio_set_value(Servo, 1);
        usleep(value);
        gpio_set_value(Servo, 0);
        usleep(23000);
    }
}

int main(int argc, char *argv[]){
    QApplication a(argc, argv);
    MainWindow w;
    w.show();

    pthread_t thread_1, thread_2, thread_3;
    int led[3] = {LED_BACK, LED_HOME, LED_MENU};

    // LED initialize
    for (int i = 0; i < 3; i++) {
        gpio_export(led[i]);
        gpio_set_dir(led[i], 1);
    }

    for (int i = 0; i < 3; i++) {;
        gpio_set_value(led[i], 0);
    }

    // thread run
    pthread_create(&thread_1, NULL, Calculate_Dist, (void *)NULL);
    pthread_create(&thread_2, NULL, MotorDrive, (void *)NULL);
    pthread_create(&thread_3, NULL, Servo_cont, (void *)NULL);

    return a.exec();
}
