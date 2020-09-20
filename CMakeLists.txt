idf_component_register(
    SRCS "source/wifi.cpp" "source/config.cpp" "source/esp32_storage.cpp" "source/mqtt.cpp" "source/device_info.cpp" "source/settings_server.cpp"
    INCLUDE_DIRS "include"
    REQUIRES nvs_flash mqtt esp_http_server json
)
