idf_component_register(SRCS "main.c" "TOTP/totp.c" "SYS_MODE/sys_mode.c" "Buttons/buttons.c" "nvs_plugin.c" "OTA/ota.c" "Camera/camera.c" "MQTT/mqtt.c" "QR/qr.c" "QR/qr_logic.c" "Screen/screen.c" "Starter/starter.c" "BT/bt.c" "BT/bt_logic.c" "common.c"
                    INCLUDE_DIRS "." 
                    REQUIRES bt
                    REQUIRES nvs_flash
                    REQUIRES app_update
                    REQUIRES mqtt
                    REQUIRES esp_http_client
                    REQUIRES esp_https_ota
                    REQUIRES mbedtls)


idf_component_get_property(quirc_lib_name espressif__quirc COMPONENT_LIB)

target_compile_options(${COMPONENT_LIB} PRIVATE -O3)
target_compile_options(${quirc_lib_name} PRIVATE -O3)

