#pragma once

#include "essentials/helpers.hpp"
#include <string_view>
#include <vector>

namespace essentials {

struct PersistentStorage {
  virtual ~PersistentStorage() = default;

  virtual int size(std::string_view key) const = 0;
  virtual std::vector<uint8_t> read(std::string_view key, int size) const = 0;
  virtual void write(std::string_view key, Span<uint8_t> data) = 0;
  virtual void clear() = 0;
};

}
