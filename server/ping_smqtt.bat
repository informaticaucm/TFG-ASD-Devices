mosquitto_pub --cafile keys/CA/rootCA.pem -d -q 1 -h "thingsboard.asd" -p "8883" -t "v1/devices/me/telemetry" -u "c9YTwKgDDaaMMA5oVv6z" -m {"temperature":25}

@REM mosquitto_pub -d -q 1 -h thingsboard.asd -p 1883 -t v1/devices/me/telemetry -u c9YTwKgDDaaMMA5oVv6z -m "{temperature:25}"