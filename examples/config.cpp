#include "essentials/config.hpp"
#include "essentials/esp32_storage.hpp"

#include "esp_system.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

extern "C"
void app_main() {
  namespace es = essentials;
  es::Esp32Storage configStorage{"config"};
  es::Config config{configStorage};

  auto ssidConfig = config.get<std::string>("ssid");
  auto integerConfig = config.get<int>("integer");

  // operator* loads value from storage and returns it
  // similar usage as std::optional
  if (ssidConfig->empty()) {
    ssidConfig = "New SSID"; // operator= saves to storage
  }

  printf("Old SSID is %s\n", ssidConfig->c_str()); // load from storage
  ssidConfig = *ssidConfig + "."; // save to storage
  printf("New SSID is %s\n", ssidConfig->c_str()); // load from storage again

  printf("Integer is %d\n", *integerConfig); // load from storage
  int integer = *integerConfig;
  // calculate stuff with integer
  integer++;
  // end of calculation
  integerConfig = integer; // save to storage

  vTaskDelay(pdMS_TO_TICKS(5000));
  esp_restart();
}
