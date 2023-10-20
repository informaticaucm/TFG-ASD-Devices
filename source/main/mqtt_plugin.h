void mqtt_send(char *topic, char *msg);
void mqtt_subscribe(char *topic);
void mqtt_init(void (*callback)(char *, char *));