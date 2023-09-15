#include "esp_heap_caps.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "esp_mac.h"
#include "essentials/device_info.hpp"
#include "freertos/FreeRTOSConfig.h"

#include <array>
#include <cstdio>

namespace essentials {

std::size_t DeviceInfo::totalHeap() const {
  multi_heap_info_t info;
  heap_caps_get_info(&info, MALLOC_CAP_INTERNAL);
  return info.total_free_bytes + info.total_allocated_bytes;
}

std::size_t DeviceInfo::freeHeap() const {
  return heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
}

std::string DeviceInfo::uniqueId() const {
  constexpr std::size_t macSize = 6;
  std::array<uint8_t, macSize> mac;
  esp_efuse_mac_get_default(mac.data());

  std::string id;
  // NOTE skip vendor part of mac address
  id.resize(macSize);
  sprintf(id.data(), "%02x%02x%02x", mac[3], mac[4], mac[5]);

  return id;
}

int64_t DeviceInfo::uptime() const {
  return esp_timer_get_time();
}

}