#include "essentials/mqtt.hpp"

#include "essentials/config.hpp"
#include "essentials/device_info.hpp"
#include "essentials/esp32_storage.hpp"
#include "essentials/settings_server.hpp"
#include "essentials/wifi.hpp"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace es = essentials;

// don't forget to link root certificate in root CMakeLists.txt:
// target_add_binary_data(${CMAKE_PROJECT_NAME}.elf "main/cert.pem" TEXT)
// see how to generate certificate below in Details section of readme.md
extern const uint8_t mqttCertBegin[] asm("_binary_cert_pem_start");
extern const uint8_t mqttCertEnd[] asm("_binary_cert_pem_end");

es::DeviceInfo deviceInfo{};

void exampleApp() {
  es::Esp32Storage configStorage{"config"};
  es::Config config{configStorage};
  auto ssid = config.get<std::string>("ssid");
  auto wifiPass = config.get<std::string>("wifiPass");

  es::Esp32Storage mqttStorage{"mqtt"};
  es::Config mqttConfig{mqttStorage};
  auto mqttUrl = mqttConfig.get<std::string>("url");
  auto mqttUser = mqttConfig.get<std::string>("user");
  auto mqttPass = mqttConfig.get<std::string>("pass");

  es::Wifi wifi;
  es::SettingsServer settingsServer{80,
    "My App",
    "1.0.1",
    {
      {"WiFi SSID", ssid},
      {"WiFi Password", wifiPass},
      {"MQTT URL", mqttUrl},
      {"MQTT Username", mqttUser},
      {"MQTT Password", mqttPass},
    }};

  wifi.connect(*ssid, *wifiPass);

  int tryCount = 0;
  while (!wifi.isConnected() && tryCount < 10) {
    tryCount++;
    printf("Waiting for connection...\n");
    vTaskDelay(pdMS_TO_TICKS(1000));
  }

  if (!wifi.isConnected()) {
    printf("Couldn't connect to the wifi. Starting WiFi AP with settings server.\n");
    wifi.startAccessPoint("esp32", "12345678", es::Wifi::Channel::Channel5);
    settingsServer.start();
    while (true) {
      printf("Waiting for configuration...\n");
      vTaskDelay(pdMS_TO_TICKS(1000));
    }
  }

  auto mqttCert =
    std::string_view{reinterpret_cast<const char*>(mqttCertBegin), std::size_t(mqttCertEnd - mqttCertBegin)};
  std::string mqttPrefix = "esp32/" + deviceInfo.uniqueId();

  std::string url = *mqttUrl;
  std::string user = *mqttUser;
  std::string pass = *mqttPass;
  printf("%s %s %s\n", url.c_str(), user.c_str(), pass.c_str());
  es::Mqtt::ConnectionInfo mqttInfo{url, mqttCert, user, pass};
  es::Mqtt::LastWillMessage lastWill{"last/will", "Last will message", es::Mqtt::Qos::Qos0, false};

  es::Mqtt mqtt{mqttInfo,
    std::string_view{mqttPrefix},
    std::chrono::seconds{30},
    lastWill,
    []() { printf("MQTT is connected!\n"); },
    []() { printf("MQTT is disconnected!\n"); }};

  // task which publishes device info every second
  xTaskCreate(
    +[](void* arg) {
      es::Mqtt& mqtt = *reinterpret_cast<es::Mqtt*>(arg);
      while (true) {
        mqtt.publish("info/freeHeap", deviceInfo.freeHeap(), es::Mqtt::Qos::Qos0, false);
        mqtt.publish("info/totalHeap", deviceInfo.totalHeap(), es::Mqtt::Qos::Qos0, false);
        mqtt.publish("info/uptime", deviceInfo.uptime(), es::Mqtt::Qos::Qos0, false);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
      }
    },
    "device_info",
    4 * 1024,
    &mqtt,
    configMAX_PRIORITIES,
    nullptr);

  std::vector<std::unique_ptr<es::Mqtt::Subscription>> subs;

  subs.emplace_back(mqtt.subscribe("ping", es::Mqtt::Qos::Qos0, [&mqtt](std::string_view data) {
    std::string text = std::string(data);
    printf("got ping: %s\n", text.c_str());
    mqtt.publish("pong", "Pinging back :)", es::Mqtt::Qos::Qos0, false);
  }));

  subs.emplace_back(
    // lambda subscription
    mqtt.subscribe<int>("number", es::Mqtt::Qos::Qos0, [&mqtt](std::optional<int> value) {
      if (value) {
        printf("got number value: %d\n", *value);
      }
    }));

  int myValue = 0;
  subs.emplace_back(
    // value subscription
    mqtt.subscribe("number", es::Mqtt::Qos::Qos0, myValue));

  std::string myText{};
  subs.emplace_back(
    // multiple subscriptions to same topic 'number'
    mqtt.subscribe("number", es::Mqtt::Qos::Qos0, myText));

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

extern "C" void app_main() {
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
