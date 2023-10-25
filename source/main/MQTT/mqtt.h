struct MQTTConf
{
    char *broker_url;
    char *device_id;
    void (*callback)(char *, char *);
};

void mqtt_send(char *topic, char *msg);
void mqtt_subscribe(char *topic);
void mqtt_init(MQTTConf conf);
void mqtt_send_ota_status_report(char *status);
void mqtt_send_ota_fail(char *explanation);
