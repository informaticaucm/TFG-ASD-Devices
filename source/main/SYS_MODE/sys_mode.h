#pragma once

#include "../Starter/starter.h"
#include <time.h>

enum sys_mode
{
    mirror,
    qr_display,
    self_managed,
};

char *sys_mode_to_string[] = {
    "mirror",
    "qr_display",
    "self_managed",
};

struct sys_mode_state
{
    int task_delay;
    int rt_task_delay;
    bool ota_running;
    enum sys_mode mode;
    enum sys_mode tmp_mode;
    int tmp_mode_expiration;
    char version[32];
    char TOTP_secret[17];
    int TOTP_t0;
    // char qr_url_template[URL_SIZE];
    bool totp_ready;
    struct ConfigurationParameters parameters;
};

enum sys_mode get_mode();
void set_mode(enum sys_mode mode);
void set_tmp_mode(enum sys_mode mode, int sec_duration, enum sys_mode next_mode);

void get_parameters(struct ConfigurationParameters *parameters);
void set_parameters(struct ConfigurationParameters *parameters);

void set_version(char version[32]);
void get_version(char version[32]);

void set_rt_task_delay(int rt_task_delay);
int get_rt_task_delay();

void set_task_delay(int task_delay);
int get_task_delay();

void set_ota_running(bool ota_running);
bool is_ota_running();

// void set_qr_url_template(char qr_url_template[URL_SIZE]);
// void get_qr_url_template(char qr_url_template[URL_SIZE]);

void set_TOTP_secret(char TOTP_secret[17]);
void get_TOTP_secret(char TOTP_secret[17]);

void set_TOTP_t0(int TOTP_t0);
int get_TOTP_t0();

void set_TOTP_ready(bool totp_ready);
bool is_totp_ready();