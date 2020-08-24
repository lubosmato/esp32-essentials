#pragma once

#include "sdkconfig.h"
#include "essentials/config.hpp"
#include <memory>

namespace essentials {

struct Wifi {
  static constexpr int MAXIMUM_CONNECT_ATTEMPTS = 10;

  static constexpr std::string_view DEFAULT_AP_SSID = CONFIG_WIFI_AP_SSID;
  static constexpr std::string_view DEFAULT_AP_PASS = CONFIG_WIFI_AP_PASS;
  static constexpr int DEFAULT_AP_CHANNEL = CONFIG_WIFI_AP_DEFAULT_CHANNEL;

  Wifi(
    Config::Value<std::string>& ssid, 
    Config::Value<std::string>& password
  );
  ~Wifi();
private:
  struct Private;
  std::unique_ptr<Private> p;
};

}
