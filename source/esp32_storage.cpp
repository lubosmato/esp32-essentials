#include "essentials/esp32_storage.hpp"

#include "esp_system.h"
#include "nvs_flash.h"

#include <stdexcept>

namespace essentials {

Esp32Storage::Esp32Storage(std::string_view name) : _name(name) {
  initialize();
}

int Esp32Storage::size(std::string_view key) const {
  size_t size = -1;
  esp_err_t error = nvs_get_blob(_nvsHandle, std::string(key).c_str(), nullptr, &size);
  if (error != ESP_OK && error != ESP_ERR_NVS_NOT_FOUND) {
    return -1;
  }
  return size;
}

std::vector<uint8_t> Esp32Storage::read(std::string_view key, int size) const {
  auto buffer = std::vector<uint8_t>{};
  buffer.resize(size);

  size_t blobSize = size;
  esp_err_t error = nvs_get_blob(_nvsHandle, std::string(key).c_str(), buffer.data(), &blobSize);
  if (error == ESP_ERR_NVS_NOT_FOUND) {
    buffer.clear();
    return buffer;
  }
  if (error != ESP_OK) {
    throw std::runtime_error("error while getting NVS blob");
  }

  return buffer;
}

void Esp32Storage::write(std::string_view key, Span<uint8_t> data) {
  esp_err_t error = nvs_set_blob(_nvsHandle, std::string(key).c_str(), data.data, data.size);
  if (error != ESP_OK) {
    throw std::runtime_error("error while writing to NVS");
  }

  error = nvs_commit(_nvsHandle);
  if (error != ESP_OK) {
    throw std::runtime_error("error while committing NVS");
  }
}

void Esp32Storage::clear() {
  esp_err_t error = nvs_erase_all(_nvsHandle);
  if (error != ESP_OK) {
    throw std::runtime_error("couldn't erase flash");
  }
  initialize();
}

void Esp32Storage::initialize() {
  esp_err_t error = nvs_flash_init();
  if (error == ESP_ERR_NVS_NO_FREE_PAGES || error == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    error = nvs_flash_erase();
    if (error != ESP_OK) {
      throw std::runtime_error("error in NVS initialization, couldn't erase flash");
    }
    error = nvs_flash_init();
  }
  if (error != ESP_OK) {
    throw std::runtime_error("error in NVS initialization, couldn't initalize");
  }

  error = nvs_open(_name.c_str(), NVS_READWRITE, &_nvsHandle);
  if (error != ESP_OK) {
    throw std::runtime_error("error while opening NVS");
  }
}

Esp32Storage::~Esp32Storage() {
  nvs_close(_nvsHandle);
}

}
