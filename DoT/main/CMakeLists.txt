idf_component_register(
    SRCS "hello_world_main.c"
    INCLUDE_DIRS "."
    REQUIRES driver
    REQUIRES esp_wifi esp_netif nvs_flash
)
