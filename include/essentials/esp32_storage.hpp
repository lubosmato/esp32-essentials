#pragma once

#include <string>
#include "essentials/persistent_storage.hpp"
#include "nvs.h"

namespace essentials {

struct Esp32Storage : PersistentStorage {  
  explicit Esp32Storage(std::string_view name);
  ~Esp32Storage();

  int size(std::string_view key) const override;
  std::vector<uint8_t> read(std::string_view key, int size) const override;
  void write(std::string_view key, Span<uint8_t> data) override;
  void clear() override;
private:
  void initialize();
  nvs_handle_t _nvsHandle;
  std::string _name;
};

}
