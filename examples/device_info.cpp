#include "essentials/device_info.hpp"

#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

extern "C" void app_main() {
  namespace es = essentials;

  es::DeviceInfo deviceInfo{};
  printf("Unique id: %s\n", deviceInfo.uniqueId().c_str());
  printf("Free heap: %d\n", deviceInfo.freeHeap());
  printf("Total heap: %d\n", deviceInfo.totalHeap());
  printf("Uptime: %lld\n", deviceInfo.uptime());

  vTaskDelay(pdMS_TO_TICKS(5000));
  esp_restart();
}
