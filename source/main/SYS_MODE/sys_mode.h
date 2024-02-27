#ifndef __SYS_MODE_H__
#define __SYS_MODE_H__
#include "../Starter/starter.h"
#include "../common.h"
#include <time.h>

enum sys_mode
{
    mirror,
    qr_display,
    state_display,
    BT_list,
    button_test,
};

struct bt_device_record
{
    char name[32];
    char address[6];
    int last_time;
    int first_time;
} ;

struct sys_mode_state
{
    int task_delay;
    int rt_task_delay;
    int idle_task_delay;
    bool ota_running;
    enum sys_mode mode;
    enum sys_mode tmp_mode;
    int tmp_mode_expiration;
    char version[32];
    bool mqtt_normal_operation;
    int last_ping_time;
    int last_tb_ping_time;

    struct bt_device_record device_history[BT_DEVICE_HISTORY_SIZE];
};

enum sys_mode get_mode();
void set_mode(enum sys_mode mode);
void set_tmp_mode(enum sys_mode mode, int sec_duration, enum sys_mode next_mode);

void set_bt_device_history(struct bt_device_record *bt_device_history);
void get_bt_device_history(struct bt_device_record *bt_device_history);

void set_version(char version[32]);
void get_version(char version[32]);

void set_rt_task_delay(int rt_task_delay);
int get_rt_task_delay();

void set_task_delay(int task_delay);
int get_task_delay();

void set_idle_task_delay(int rt_task_delay);
int get_idle_task_delay();

void set_ota_running(bool ota_running);
bool is_ota_running();

void set_mqtt_normal_operation(bool mqtt_normal_operation);
bool is_mqtt_normal_operation();

void set_last_ping_time(int last_ping_time);
int get_last_ping_time();

void set_last_tb_ping_time(int last_ping_time);
int get_last_tb_ping_time();

#endif