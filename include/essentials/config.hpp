#pragma once

#include <string>
#include "essentials/persistent_storage.hpp"

namespace essentials {

class Config {

protected:
  PersistentStorage& _storage;

public:
  Config(PersistentStorage& storage);

  template<typename T>
  class Value {
    static_assert(
      std::is_integral_v<T> || std::is_floating_point_v<T> || std::is_same_v<T, std::string>, 
      "Only integral, floating point types and std::string type are allowed"
    );

    Config& _config;
    T _value;
    T _defaultValue;
    std::string _key;
    static constexpr int _dataSize = sizeof(T);
    
    friend class Config;

    Value(Config& config, std::string_view key, T defaultValue) : _config(config), _value(defaultValue), _defaultValue(defaultValue), _key(key) {}

    void _load() {
      if constexpr (std::is_same_v<T, std::string>) {
        int size = _config._storage.size(_key);
        if (size < 0) {
          _value = _defaultValue;
          _save();
        } else {
          std::vector<uint8_t> data = _config._storage.read(_key, size);
          _value = std::string(data.begin(), data.end());
        }
      } else {
        std::vector<uint8_t> data = _config._storage.read(_key, _dataSize);
        if (data.empty()) {
          _value = _defaultValue;
          _save();
        } else {
          _value = *reinterpret_cast<T*>(data.data());
        }
      }
    }

    void _save() {
      if constexpr (std::is_same_v<T, std::string>) {
        _config._storage.write(_key, { reinterpret_cast<uint8_t*>(_value.data()), _value.size() });
      } else {
        _config._storage.write(_key, { reinterpret_cast<uint8_t*>(&_value), _dataSize });
      }
    }

  public:
    T operator*() {
      _load();
      return _value;
    }
    
    const T* operator->() {
      _load();
      return &_value;
    }

    Value& operator=(const T& newValue) {
      _value = newValue;
      _save();
      return *this;
    }
  };
  
  template<typename T>
  Value<T> get(std::string_view key, T defaultValue = T{}) {
    return Value<T>{*this, key, defaultValue};
  }

};

}
