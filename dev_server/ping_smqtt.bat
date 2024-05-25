mosquitto_pub --cafile keys/CA/rootCA.pem -d -q 1 -h "thingsboard.asd" -u "h3wtSyoGrmYlBLCL0qOX" -p "8883" -t "v1/devices/me/telemetry" -f ping.json
