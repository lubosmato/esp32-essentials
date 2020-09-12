#pragma once

#include <memory>
#include <string_view>
#include <string>
#include "essentials/config.hpp"

namespace essentials {

struct SettingsServer {
  struct Field {
    std::string label;
    Config::Value<std::string>& value;
  };

  SettingsServer(uint16_t port, std::string_view deviceName, std::string_view version, std::vector<Field> fields);
  ~SettingsServer();

  void start();
  void stop();

private:
  struct Private;
  std::unique_ptr<Private> p;
};

}
