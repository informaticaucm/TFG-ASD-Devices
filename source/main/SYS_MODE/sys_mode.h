#define DEFAULT_SYS_MODE mirror

enum sys_mode
{
    mirror,
    qr_display,
    self_managed,
};

struct sys_mode_state
{
    enum sys_mode mode;
};

enum sys_mode get_mode();
void set_mode(enum sys_mode mode);
