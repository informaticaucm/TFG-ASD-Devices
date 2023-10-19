idf_component_register(SRCS "mqtt_plugin.c" "sync.c" "https_ota.c" "main.c" "mqtt_plugin.c"
                    INCLUDE_DIRS "."
                    # Embed the server root certificate into the final binary
                    EMBED_TXTFILES ${project_dir}/server_certs/ca_cert.pem)
