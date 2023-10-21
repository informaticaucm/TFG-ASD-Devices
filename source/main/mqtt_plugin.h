void mqtt_send(char *topic, char *msg);
void mqtt_subscribe(char *topic);
void mqtt_init(void (*callback)(char *, char *));
void mqtt_send_ota_status_report(char *status);
void mqtt_send_ota_fail(char *explanation);
