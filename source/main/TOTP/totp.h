
struct TOTPConf
{
    QueueHandle_t to_screen_queue;
};

void start_totp(struct TOTPConf *conf);