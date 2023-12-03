#pragma once
#include "../Starter/starter.h"
#include <time.h>

enum sys_mode
{
    mirror,
    qr_display,
    self_managed,
};

struct sys_mode_state
{
    enum sys_mode mode;
    enum sys_mode tmp_mode;
    int tmp_mode_expiration;
    char version[32];
    struct ConfigurationParameters parameters;
};

enum sys_mode get_mode();
void set_mode(enum sys_mode mode);
void set_tmp_mode(enum sys_mode mode, int sec_duration, enum sys_mode next_mode);
void get_parameters(struct ConfigurationParameters *parameters);
void set_parameters(struct ConfigurationParameters *parameters);
void set_version(char version[32]);
void get_version(char version[32]);