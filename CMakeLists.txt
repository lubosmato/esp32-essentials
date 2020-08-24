idf_component_register(
    SRCS "source/wifi.cpp" "source/config.cpp" "source/esp32_storage.cpp" "source/mqtt.cpp" "source/esp32_device_info.cpp"
    INCLUDE_DIRS "include"
    REQUIRES nvs_flash mqtt esp_http_server
)
