#include "essentials/esp32_device_info.hpp"

#include "freertos/FreeRTOSConfig.h"
#include "esp_system.h"
#include "esp_heap_caps.h"

#include <array>
#include <cstdio>

namespace essentials {

std::size_t Esp32DeviceInfo::usedHeap() const {
  multi_heap_info_t info;
  heap_caps_get_info(&info, MALLOC_CAP_DEFAULT);
  return info.total_allocated_bytes;
}

std::size_t Esp32DeviceInfo::freeHeap() const {
  multi_heap_info_t info;
  heap_caps_get_info(&info, MALLOC_CAP_DEFAULT);
  return info.total_free_bytes;
}

std::string Esp32DeviceInfo::uniqueId() const {
  constexpr std::size_t macSize = 6;
  std::array<uint8_t, macSize> mac;
  esp_efuse_mac_get_default(mac.data());
  
  std::string id;
  id.resize(macSize * 2);
  sprintf(id.data(), "%02x%02x%02x%02x%02x%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  
  return id;
}

}