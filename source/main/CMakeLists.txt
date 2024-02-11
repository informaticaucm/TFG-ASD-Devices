idf_component_register(SRCS "main.c" "TOTP/totp.c" "SYS_MODE/sys_mode.c" "Buttons/buttons.c" "nvs_plugin.c" "OTA/ota.c" "Camera/camera.c" "MQTT/mqtt.c" "QR/qr.c" "Screen/screen.c" "Starter/starter.c" "BT/bt.c" "common.c"
                    INCLUDE_DIRS "." 
                    # Embed the server root certificate into the final binary
                    EMBED_TXTFILES ${project_dir}/server_certs/ca_cert.pem)


idf_component_get_property(quirc_lib_name espressif__quirc COMPONENT_LIB)

target_compile_options(${COMPONENT_LIB} PRIVATE -O3)
target_compile_options(${quirc_lib_name} PRIVATE -O3)

