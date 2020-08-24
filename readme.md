# Essentials for ESP32
C++17 ESP-IDF component with boilerplate for WiFi, MQTT, configuration, persitent storage and device info.

# How to use
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
2. Add binary data of web settings app into root `CMakeLists.txt`:
```cmake
cmake_minimum_required(VERSION 3.5)

set(CMAKE_CXX_STANDARD 20)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(my-esp-idf-project)

target_add_binary_data(${CMAKE_PROJECT_NAME}.elf "components/essentials/resources/web/dist/app.js.gz" TEXT)
target_add_binary_data(${CMAKE_PROJECT_NAME}.elf "components/essentials/resources/web/dist/index.html.gz" TEXT)
```
3. Add `REQUIRES` into your main `CMakeLists.txt`:
```
idf_component_register(
    SRCS "main.cpp"
    REQUIRES essentials
)
```
4. (optional) Configure AP WiFi settings (ssid, password, channel) with `idf.py menuconfig`
5. Build

# Examples

## Device info
Provides access to device internal information such as free/used heap, unique ID, ...

Example:
```cpp
#include "essentials/esp32_device_info.hpp"

namespace es = essentials;

es::Esp32DeviceInfo deviceInfo{};
printf("Unique id: %s\n", deviceInfo.uniqueId().c_str());
printf("Free heap: %d\n", deviceInfo.freeHeap());
printf("Used heap: %d\n", deviceInfo.usedHeap());
```

## Storage
Storage which uses NVS.

Example:
```cpp
#include "essentials/esp32_storage.hpp"

namespace es = essentials;

// Esp32Storage uses nvs to store data
es::Esp32Storage storage{"storageKey"};
storage.clear();

std::vector<uint8_t> data = storage.read("mac", 6); // read 6-byte value from storage with key 'mac' 
if (data.empty()) {
  data.resize(6);
  for (uint8_t& b : data)
    b = uint8_t(esp_random());
  storage.write("mac", es::Range<uint8_t>{data.data(), data.size()});
}
```

## Config
Provides convenient way for storing/loading configuration values. Uses `essentials::PersistentStorage`.

Example:
```cpp
#include "essentials/esp32_storage.hpp"
#include "essentials/config.hpp"

namespace es = essentials;

es::Esp32Storage configStorage{"config"};
es::Config config{configStorage};

auto ssidConfig = config.get<std::string>("ssid");
auto integerConfig = config.get<int>("integer");

printf("Old SSID is %s\n", *ssidConfig); // load from storage
ssidConfig = "New SSID"; // saves into storage
printf("New SSID is %s\n", *ssidConfig); // load from storage again

printf("Integer is %d\n", *integerConfig); // load from storage
int integer = integerConfig; // load from storage
// ... calculate stuff with integer ...
integerConfig = integer; // save to storage
```

## WiFi
WiFi is managed by RAII class which tries to connect to the WiFi based on given credentials. 

If connection is not possible, it automatically switches to `AP` mode and runs web server with settings web interface. New settings are stored into the storage with use of `essentials::Config` values. Settings can be adjusted on `http://192.168.4.1/`:
![Web Settings](data:image/png;base64,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)

Example:
```cpp
#include "essentials/esp32_storage.hpp"
#include "essentials/config.hpp"
#include "essentials/wifi.hpp"

namespace es = essentials;
// ...
es::Esp32Storage configStorage{"config"};
es::Config config{configStorage};

auto ssid = config.get<std::string>("ssid");
auto wifiPass = config.get<std::string>("wifiPass");
es::Wifi wifi{ssid, wifiPass}; // blocking call, waiting for connection
```

## MQTT
```cpp
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "essentials/wifi.hpp"
#include "essentials/mqtt.hpp"
#include "essentials/esp32_storage.hpp"
#include "essentials/esp32_device_info.hpp"
#include "essentials/config.hpp"

namespace es = essentials;

extern const uint8_t mqttCertBegin[] asm("_binary_cert_pem_start");
extern const uint8_t mqttCertEnd[] asm("_binary_cert_pem_end");

es::Esp32DeviceInfo deviceInfo{};

void exampleApp() {
  es::Esp32Storage configStorage{"config"};
  es::Config config{configStorage};

  auto ssid = config.get<std::string>("ssid");
  auto wifiPass = config.get<std::string>("wifiPass");
  es::Wifi wifi{ssid, wifiPass};

  std::string mqttPrefix = "esp32/" + deviceInfo.uniqueId();
  es::Mqtt::ConnectionInfo mqttInfo{
    "mqtts://my.mqtt.com:8883",
    std::string_view{
      reinterpret_cast<const char*>(mqttCertBegin),
      std::size_t(mqttCertEnd - mqttCertBegin)
    },
    "username",
    "password"
  };
  es::Mqtt mqtt{mqttInfo, mqttPrefix};

  // task which publishes device info every second
  xTaskCreate(
    +[](void* arg) {
      es::Mqtt& mqtt = *reinterpret_cast<es::Mqtt*>(arg);
      while (true) {
        mqtt.publish("info/freeHeap", deviceInfo.freeHeap(), es::Mqtt::Qos::Qos0, false);
        mqtt.publish("info/usedHeap", deviceInfo.usedHeap(), es::Mqtt::Qos::Qos0, false);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
      }
    },
    "device_info",
    4 * 1024,
    &mqtt,
    configMAX_PRIORITIES,
    nullptr
  );

  std::vector<std::unique_ptr<es::Mqtt::Subscription>> subs;

  subs.emplace_back(
    mqtt.subscribe("ping", es::Mqtt::Qos::Qos0, [&mqtt](std::string_view data) {
      std::string text = std::string(data);
      printf("got ping: %s\n", text.c_str());
      mqtt.publish("pong", "Pinging back :)", es::Mqtt::Qos::Qos0, false);
    })
  );

  subs.emplace_back(
    // lambda subscription
    mqtt.subscribe<int>("number", es::Mqtt::Qos::Qos0, [&mqtt](std::optional<int> value) {
      if (value) {
        printf("got number value: %d\n", *value);
      }
    })
  );
  
  int myValue = 0;
  subs.emplace_back(
    // value subscription
    mqtt.subscribe("number", es::Mqtt::Qos::Qos0, myValue)
  );

  std::string myText{};
  subs.emplace_back(
    // multiple subscriptions to same topic 'number'
    mqtt.subscribe("number", es::Mqtt::Qos::Qos0, myText)
  );

  int seconds = 0;
  while (true) {
    printf("myValue: %d\n", myValue);
    printf("myText: %s\n", myText.c_str());

    if (seconds % 10 == 0) {
      mqtt.publish("test/string", "how are you?", es::Mqtt::Qos::Qos0, false);
      mqtt.publish("test/integer", 42, es::Mqtt::Qos::Qos0, false);
      mqtt.publish("test/double", 42.4242, es::Mqtt::Qos::Qos0, false);
      mqtt.publish("test/bool", true, es::Mqtt::Qos::Qos0, false);
    }

    vTaskDelay(1000 / portTICK_PERIOD_MS);
    ++seconds;
  }
}

extern "C"
void app_main()
{
  try {
    exampleApp();
  } catch (const std::exception& e) {
    printf("EXCEPTION: %s\n", e.what());
  } catch (...) {
    printf("UNKNOWN EXCEPTION\n");
  }
  vTaskDelay(5000 / portTICK_PERIOD_MS);
  esp_restart();
}
```

## Details
Binary has approximately 1MB thus you might need to adjust main partition size in `partitions.csv`:
```
# ESP-IDF Partition Table
# Name,   Type, SubType, Offset,  Size, Flags
nvs,      data, nvs,     0x9000,  0x6000,
phy_init, data, phy,     0xf000,  0x1000,
factory,  app,  factory, 0x10000, 2M, # <--
```

Library uses exceptions and C++17 features thus you might need to enable them.

Good app for testing MQTT: http://mqtt-explorer.com/
