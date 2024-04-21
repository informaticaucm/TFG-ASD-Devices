#ifndef __SYS_MODE_H__
#define __SYS_MODE_H__
#include "../Starter/starter.h"
#include "../common.h"
#include <time.h>

enum ScreenMode
{
    mirror,
    qr_display,
    msg_display,
    button_test,

};

#define VALID_ENTRY(x) ((x.valid) && (time(0) - x.last_time) < BT_DEVICE_HISTORY_MAX_AGE)

struct bt_device_record
{
    bool valid;
    char name[32];
    char address[6];
    int last_time;
    int first_time;
};

struct sys_mode_state
{
    int task_delay;
    int rt_task_delay;
    int idle_task_delay;
    int ping_delay;
    bool ota_running;
    char totp_form_base_url[URL_SIZE];
    enum ScreenMode mode;
    enum ScreenMode tmp_mode;
    int tmp_mode_expiration;
    char version[32];
    bool mqtt_normal_operation;
    int last_ping_time;
    int last_tb_ping_time;

    struct bt_device_record device_history[BT_DEVICE_HISTORY_SIZE];
};

enum ScreenMode get_mode();
enum ScreenMode get_notmp_mode();
void set_mode(enum ScreenMode mode);
void set_tmp_mode(enum ScreenMode mode, int sec_duration, enum ScreenMode next_mode);

void set_bt_device_history(struct bt_device_record bt_device_history[BT_DEVICE_HISTORY_SIZE]);
void get_bt_device_history(struct bt_device_record bt_device_history[BT_DEVICE_HISTORY_SIZE]);

void set_version(char version[32]);
void get_version(char version[32]);

void set_totp_form_base_url(int totp_form_base_url);
int get_totp_form_base_url();

void set_rt_task_delay(int rt_task_delay);
int get_rt_task_delay();

void set_task_delay(int task_delay);
int get_task_delay();

void set_idle_task_delay(int rt_task_delay);
int get_idle_task_delay();

void set_ping_delay(int ping_delay);
int get_ping_delay();

void set_ota_running(bool ota_running);
bool is_ota_running();

void set_mqtt_normal_operation(bool mqtt_normal_operation);
bool is_mqtt_normal_operation();

void set_last_ping_time(int last_ping_time);
int get_last_ping_time();

void set_last_tb_ping_time(int last_ping_time);
int get_last_tb_ping_time();

#endif