#include "essentials/wifi.hpp"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

extern "C" void app_main() {
  namespace es = essentials;

  es::Wifi wifi;

  wifi.setConnectCallback([]() { printf("Successfully connected to wifi\n"); });
  wifi.setDisconnectCallback([]() { printf("Disconnected from wifi\n"); });

  wifi.connect("My SSID", "my password");

  int tryCount = 0;
  while (!wifi.isConnected() && tryCount < 10) {
    tryCount++;
    printf("Waiting for connection...\n");
    vTaskDelay(pdMS_TO_TICKS(1000));
  }

  if (!wifi.isConnected()) {
    printf("Couldn't connect to the wifi. Starting WiFi AP.\n");
    wifi.startAccessPoint("esp32", "12345678", es::Wifi::Channel::Channel5);
  }

  while (true) {
    std::optional<es::Ipv4Address> ip = wifi.ipv4();
    if (ip) {
      printf("IP: %s\n", ip->toString().c_str());
    } else {
      printf("Don't have IP\n");
    }
    vTaskDelay(pdMS_TO_TICKS(5000));
  }
}
