#pragma once

#include <string>

namespace essentials {

struct DeviceInfo {
  std::size_t totalHeap() const;
  std::size_t freeHeap() const;
  std::string uniqueId() const;
  int64_t uptime() const;
};

}
