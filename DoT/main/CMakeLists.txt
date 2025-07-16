idf_component_register(
    SRCS "hello_world_main.c"
    INCLUDE_DIRS "."
    REQUIRES esp_wifi esp_event nvs_flash esp_websocket_client json driver
)
