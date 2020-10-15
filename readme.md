# Essentials for ESP32
C++17 ESP-IDF v4.1 component with boilerplate for WiFi, MQTT, configuration, persitent storage and device info.

# How to use

0. Download esp-idf v4.1. There is a handy [extension](https://marketplace.visualstudio.com/items?itemName=espressif.esp-idf-extension) for `VSCode` which installs all stuff you need.

1. Add this repo as a submodule into `components` folder:
    ```bash
    cd my-esp-idf-project/
    git submodule add https://github.com/lubosmato/esp32-essentials.git components/essentials/
    ```
    ```
    my-esp-idf-project/
    ├── components/
    │   ├── essentials/ <--
    │   └── ...another components...
    ├── main/
    │   ├── CMakeLists.txt
    │   └── main.cpp
    ├── CMakeLists.txt
    ├── sdkconfig
    └── partitions.csv
    ```
2. Add binary data of web settings app and C++20 support into root `CMakeLists.txt`:
    ```cmake
    cmake_minimum_required(VERSION 3.5)

    set(CMAKE_CXX_STANDARD 20) # <--

    include($ENV{IDF_PATH}/tools/cmake/project.cmake)
    project(my-esp-idf-project)

    target_add_binary_data(${CMAKE_PROJECT_NAME}.elf "components/essentials/resources/web/dist/app.js.gz" TEXT) # <-- (only needed by `essentials::SettingsServer`)
    target_add_binary_data(${CMAKE_PROJECT_NAME}.elf "components/essentials/resources/web/dist/index.html.gz" TEXT) # <-- (only needed by `essentials::SettingsServer`)
    ```
3. Enable exceptions in `idf.py menuconfig`
4. Add `REQUIRES` into your main `CMakeLists.txt`:
    ```
    idf_component_register(
        SRCS "main.cpp"
        REQUIRES essentials
    )
    ```
5. Build
6. Inspire from [examples](examples/)

[Details](examples/readme.md#Details)

# TODO
- [x] Migrate to esp-idf v4.1
- [x] Fix Esp32Storage::clear() - it mustn't clear all NVS
- [x] Add wait for MQTT connection feature with timeout (similar as Wifi)
- [ ] Add more device info
- [ ] MQTT subscription to multi and single level (heavy feature, maybe YAGNI)
- [ ] Check all error codes and throw
- [x] Make settings web server simpler without enormous number of route handlers
- [x] Make settings web app smaller (overkilled by Vue.js)
- [ ] Add tests
- [ ] Use `std::span` when will be supported by esp-idf toolchain
- [ ] Use `std::to_chars` and `std::from_chars` for floating point types when will be implemented in GCC
