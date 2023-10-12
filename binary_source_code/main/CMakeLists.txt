# Embed the server root certificate into the final binary
idf_build_get_property(project_dir PROJECT_DIR)
idf_component_register(SRCS "main.c" "../../common/connect_wifi.c" "../../common/nvs_plugin.c" "../../common/ota_version_fetcher.c"
                    INCLUDE_DIRS "."
                    INCLUDE_DIRS "../../common"
                   EMBED_TXTFILES ${project_dir}/../common/server_certs/ca_cert.pem)
