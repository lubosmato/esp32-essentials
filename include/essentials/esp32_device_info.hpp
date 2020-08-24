#pragma once

#include "essentials/device_info.hpp"

namespace essentials {

struct Esp32DeviceInfo : DeviceInfo {
  std::size_t usedHeap() const override;
  std::size_t freeHeap() const override;
  std::string uniqueId() const override;
};

}
