idf_component_register(SRCS "main.c" "Camera/camera.c" "MQTT/mqtt.c" "QR/qr.c" "Screen/screen.c" "Starter/starter.c"
                    INCLUDE_DIRS "."
                    # Embed the server root certificate into the final binary
                    EMBED_TXTFILES ${project_dir}/server_certs/ca_cert.pem)
