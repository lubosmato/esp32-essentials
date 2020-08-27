#pragma once

#include "sdkconfig.h"
#include "essentials/config.hpp"
#include <memory>

namespace essentials {

struct Wifi {
  static constexpr int CONNECTION_TIMEOUT = 15000; // [ms]

  static constexpr std::string_view DEFAULT_AP_SSID = CONFIG_WIFI_AP_SSID;
  static constexpr std::string_view DEFAULT_AP_PASS = CONFIG_WIFI_AP_PASS;
  static constexpr int DEFAULT_AP_CHANNEL = CONFIG_WIFI_AP_DEFAULT_CHANNEL;

  Wifi(
    Config::Value<std::string>& ssid,
    Config::Value<std::string>& password,
    std::string_view deviceName = "ESP32",
    std::string_view version = "1.0.0"
  );
  ~Wifi();
  bool isConnected() const;
private:
  struct Private;
  std::unique_ptr<Private> p;
};

}
