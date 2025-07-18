idf_component_register(
  SRCS "hello_world_main.c"
  INCLUDE_DIRS "."
  EMBED_FILES "ca_cert.pem"
  REQUIRES esp_websocket_client json esp_wifi esp_event nvs_flash driver
)
