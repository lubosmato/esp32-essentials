#pragma once

#include <string>

namespace essentials {

struct DeviceInfo {
  virtual ~DeviceInfo() = default;

  virtual std::size_t usedHeap() const = 0;
  virtual std::size_t freeHeap() const = 0;
  virtual std::string uniqueId() const = 0;
};

}
