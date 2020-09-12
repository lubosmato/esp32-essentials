#include "essentials/esp32_storage.hpp"

#include "esp_system.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

extern "C"
void app_main() {
  namespace es = essentials;
  // Esp32Storage uses nvs to store data
  es::Esp32Storage storage{"storageKey"};
  storage.clear();

  std::vector<uint8_t> data = storage.read("mac", 6); // read 6-byte value from storage with key 'mac' 
  if (data.empty()) {
    data.resize(6);
    for (uint8_t& b : data)
      b = uint8_t(esp_random());
    storage.write("mac", es::Span<uint8_t>{data.data(), data.size()});
  }
  for (uint8_t& b : data)
    printf("%02x ", b);
  printf("\n");

  vTaskDelay(pdMS_TO_TICKS(5000));
  esp_restart();
}
