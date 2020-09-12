#include "essentials/wifi.hpp"

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

#include "esp_wifi.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include <cstring>

namespace essentials {

const char* TAG_WIFI = "wifi";

struct Wifi::Private {
  enum class Mode {
    None = 0, Station, Ap
  };

  EventGroupHandle_t wifiEvents = xEventGroupCreate();
  int retryAttempts = 0;
  Mode mode{Mode::None};
  bool isConnected = false;
  std::optional<Ipv4Address> stationIp{};

  static constexpr int WIFI_CONNECTED_BIT = BIT0;
  static constexpr int WIFI_FAIL_BIT = BIT1;

  void connect(std::string_view ssid, std::string_view password, int connectionTimeout) {
    disconnect();
    mode = Mode::Station;

    ESP_LOGI(TAG_WIFI, "connecting to wifi");

    nvs_flash_init();
    tcpip_adapter_init();
    esp_event_loop_create_default();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    auto error = esp_wifi_init(&cfg);
    ESP_ERROR_CHECK(error);

    error |= esp_event_handler_register(
      WIFI_EVENT, 
      ESP_EVENT_ANY_ID, 
      &Private::stationEventHandler, 
      this
    );
    ESP_ERROR_CHECK(error);
    error |= esp_event_handler_register(
      IP_EVENT, 
      IP_EVENT_STA_GOT_IP, 
      &Private::stationEventHandler, 
      this
    );
    ESP_ERROR_CHECK(error);

    wifi_config_t wifiConfig{};
    std::memcpy(wifiConfig.sta.ssid, ssid.data(), ssid.size());
    std::memcpy(wifiConfig.sta.password, password.data(), password.size());

    wifiConfig.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    wifiConfig.sta.pmf_cfg.capable = true;
    wifiConfig.sta.pmf_cfg.required = false;

    error |= esp_wifi_set_mode(WIFI_MODE_STA);
    ESP_ERROR_CHECK(error);
    error |= esp_wifi_set_config(ESP_IF_WIFI_STA, &wifiConfig);
    ESP_ERROR_CHECK(error);
    error |= esp_wifi_start();
    ESP_ERROR_CHECK(error);

    std::string ssidToPrint{ssid};
    std::string passwordToPrint{password};
    ESP_LOGI(
      TAG_WIFI, 
      "trying to connect to AP SSID: '%s', password: '%s'...",
      ssidToPrint.c_str(), 
      passwordToPrint.c_str()
    );

    EventBits_t bits = xEventGroupWaitBits(
      wifiEvents,
      WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
      pdFALSE,
      pdFALSE,
      pdMS_TO_TICKS(connectionTimeout)
    );

    if (bits & WIFI_CONNECTED_BIT) {
      ESP_LOGI(
        TAG_WIFI, 
        "connected to ap SSID: '%s', password: '%s'",
        ssidToPrint.c_str(), 
        passwordToPrint.c_str()
      );
      isConnected = true;
    } else if (bits & WIFI_FAIL_BIT) {
      ESP_LOGW(
        TAG_WIFI, 
        "failed to connect to SSID: '%s', password: '%s'",
        ssidToPrint.c_str(), 
        passwordToPrint.c_str()
      );
      isConnected = false;
    } else {
      ESP_LOGW(TAG_WIFI, "wifi connection timed out");
      isConnected = false;
    }
  }

  static void stationEventHandler(
    void* arg, 
    esp_event_base_t eventBase, 
    int32_t eventId, 
    void* eventData
  ) {
    Private* p = static_cast<Private*>(arg);
    if (eventBase == WIFI_EVENT && eventId == WIFI_EVENT_STA_START) {
      esp_wifi_connect();
    } else if (eventBase == WIFI_EVENT && eventId == WIFI_EVENT_STA_DISCONNECTED) {
      esp_wifi_connect();
      ESP_LOGI(TAG_WIFI, "re-trying to connect to the AP");
    } else if (eventBase == IP_EVENT && eventId == IP_EVENT_STA_GOT_IP) {
      ip_event_got_ip_t* event = (ip_event_got_ip_t*)eventData;
      p->stationIp = Ipv4Address{event->ip_info.ip.addr};
      ESP_LOGI(TAG_WIFI, "got ip: %s", p->stationIp->toString().c_str());
      xEventGroupSetBits(p->wifiEvents, WIFI_CONNECTED_BIT);
    }
  }

  void startAccessPoint(std::string_view ssid, std::string_view password, Channel channel) {
    disconnect();
    mode = Mode::Ap;

    tcpip_adapter_init();
    esp_event_loop_create_default();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    auto error = esp_wifi_init(&cfg);

    error |= esp_event_handler_register(
      WIFI_EVENT, 
      ESP_EVENT_ANY_ID, 
      &Private::apEventHandler, 
      this
    );
    
    std::string ssidToPrint{ssid};
    std::string passwordToPrint{password};

    ESP_LOGI(
      TAG_WIFI, 
      "initializing AP SSID: '%s', password: '%s', channel: '%d'",
      ssidToPrint.c_str(), 
      passwordToPrint.c_str(), 
      uint8_t(channel)
    );

    wifi_config_t wifiConfig{};
    
    std::memcpy(wifiConfig.ap.ssid, ssid.data(), ssid.size());
    std::memcpy(wifiConfig.ap.password, password.data(), password.size());
    wifiConfig.ap.ssid_len = ssid.size();
    wifiConfig.ap.channel = uint8_t(channel);
    wifiConfig.ap.max_connection = 4;
    wifiConfig.ap.authmode = WIFI_AUTH_WPA_WPA2_PSK;

    error |= esp_wifi_set_mode(WIFI_MODE_AP);
    error |= esp_wifi_set_config(ESP_IF_WIFI_AP, &wifiConfig);
    error |= esp_wifi_start();
    ESP_ERROR_CHECK(error);
  }

  static void apEventHandler(
    void* arg, 
    esp_event_base_t eventBase, 
    int32_t eventId, 
    void* eventData
  ) {
    // Private* p = static_cast<Private*>(arg);

    if (eventId == WIFI_EVENT_AP_STACONNECTED) {
      wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) eventData;
      ESP_LOGI(TAG_WIFI, "station %02x:%02x:%02x:%02x:%02x:%02x join, AID=%d", MAC2STR(event->mac), event->aid);
    } else if (eventId == WIFI_EVENT_AP_STADISCONNECTED) {
      wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) eventData;
      ESP_LOGI(TAG_WIFI, "station %02x:%02x:%02x:%02x:%02x:%02x leave, AID=%d", MAC2STR(event->mac), event->aid);
    }
  }

  void disconnect() {
    mode = Mode::None;
    ESP_LOGI(TAG_WIFI, "disconnecting wifi");

    esp_event_handler_unregister(
      WIFI_EVENT,
      ESP_EVENT_ANY_ID,
      &Private::stationEventHandler
    );
    esp_event_handler_unregister(
      IP_EVENT,
      IP_EVENT_STA_GOT_IP,
      &Private::stationEventHandler
    );
    esp_event_handler_unregister(
      WIFI_EVENT,
      ESP_EVENT_ANY_ID,
      &Private::apEventHandler
    );

    esp_wifi_disconnect();
    esp_wifi_stop();
    esp_wifi_deinit();

    stationIp = std::nullopt;
    isConnected = false;
  }

  ~Private() {
    disconnect();
  }
};

std::string Ipv4Address::toString() const {
  std::string ip;
  const uint8_t* parts = reinterpret_cast<const uint8_t*>(&raw);
  ip += std::to_string(parts[0]);
  ip += ".";
  ip += std::to_string(parts[1]);
  ip += ".";
  ip += std::to_string(parts[2]);
  ip += ".";
  ip += std::to_string(parts[3]);
  return ip;
}

Wifi::Wifi() : p(std::make_unique<Private>()) { }

Wifi::~Wifi() = default;

bool Wifi::connect(std::string_view ssid, std::string_view password, int connectionTimeout) {
  p->connect(ssid, password, connectionTimeout);
  return p->isConnected;
}

void Wifi::disconnect() {
  p->disconnect();
}

bool Wifi::isConnected() const {
  return p->isConnected;
}

std::optional<Ipv4Address> Wifi::ipv4() const {
  switch (p->mode) {
    case Private::Mode::Ap:
      tcpip_adapter_ip_info_t info;
      if (tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_AP, &info) != ESP_OK)
        return std::nullopt;
      return Ipv4Address{info.ip.addr};

    case Private::Mode::Station:
      return p->stationIp;
    
    case Private::Mode::None:
      return std::nullopt;
  }
  return std::nullopt;
}

void Wifi::startAccessPoint(std::string_view ssid, std::string_view password, Channel channel) {
  p->startAccessPoint(ssid, password, channel);
}

}
