idf_component_register(SRCS "app.c" "../../common/connect_wifi.c" "ota_plugin.c" "../../common/nvs_plugin.c" "../../common/ota_version_fetcher.c"
                    INCLUDE_DIRS "."
                    INCLUDE_DIRS "../../common"
                    EMBED_TXTFILES ${project_dir}/../common/server_certs/ca_cert.pem)