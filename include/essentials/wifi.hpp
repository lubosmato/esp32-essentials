#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

namespace essentials {

struct Ipv4Address {
  uint32_t raw;
  std::string toString() const;
};

struct Wifi {
  enum class Channel : uint8_t { Channel1 = 1, Channel2, Channel3, Channel4, Channel5, Channel6, Channel7 };

  Wifi();
  ~Wifi();

  void setConnectCallback(std::function<void()> callback);
  void setDisconnectCallback(std::function<void()> callback);

  bool connect(std::string_view ssid, std::string_view password);
  void disconnect();
  bool isConnected() const;

  std::optional<Ipv4Address> ipv4() const;
  std::optional<int> rssi() const;

  void startAccessPoint(std::string_view ssid, std::string_view password, Channel channel);

private:
  struct Private;
  std::unique_ptr<Private> p;
};

}
