#include "esp_system.h"
#include "essentials/esp32_storage.hpp"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <array>

extern "C" void app_main() {
  namespace es = essentials;
  // Esp32Storage uses nvs to store data
  es::Esp32Storage storage{"storageKey"};
  es::Esp32Storage otherStorage{"differentKey"};
  storage.clear();
  otherStorage.clear();

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

  std::array<uint8_t, 5> dummyData{1, 2, 3, 4, 5};
  otherStorage.write("wifi", es::Span<uint8_t>{dummyData.data(), dummyData.size()});
  storage.clear();

  std::vector<uint8_t> readDummyData = otherStorage.read("wifi", dummyData.size());
  for (uint8_t& b : readDummyData)
    printf("%02x ", b);
  printf("\n");

  vTaskDelay(pdMS_TO_TICKS(5000));
  esp_restart();
}
