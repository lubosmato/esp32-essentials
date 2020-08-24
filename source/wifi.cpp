#include "essentials/wifi.hpp"
#include "essentials/version.hpp"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sys.h"
#include "esp_http_server.h"

#include <functional>
#include <cstring>
#include <array>
#include <optional>

namespace essentials {

const char* TAG_WIFI = "wifi";

extern const uint8_t indexHtmlBegin[] asm("_binary_index_html_gz_start");
extern const uint8_t indexHtmlEnd[] asm("_binary_index_html_gz_end");

extern const uint8_t appJsBegin[] asm("_binary_app_js_gz_start");
extern const uint8_t appJsEnd[] asm("_binary_app_js_gz_end");

struct Wifi::Private {
  Config::Value<std::string>& ssid;
  Config::Value<std::string>& password;

  EventGroupHandle_t wifiEvents = xEventGroupCreate();
  int retryAttempts = 0;
  bool isConnected = false;
  httpd_handle_t server = nullptr;

  static constexpr int WIFI_CONNECTED_BIT = BIT0;
  static constexpr int WIFI_FAIL_BIT = BIT1;

  std::array<httpd_uri_t, 8> handlerDefinitions{
    httpd_uri_t{
      "/", HTTP_GET, &Private::getIndex, this
    },
    httpd_uri_t{
      "/reset", HTTP_GET, &Private::getReset, this
    },
    httpd_uri_t{
      "/app.js", HTTP_GET, &Private::getAppJs, this
    },
    httpd_uri_t{
      "/config/ssid", HTTP_GET, &Private::getSsid, this
    },
    httpd_uri_t{
      "/config/ssid", HTTP_POST, &Private::setSsid, this
    },
    httpd_uri_t{
      "/config/pass", HTTP_GET, &Private::getPassword, this
    },
    httpd_uri_t{
      "/config/pass", HTTP_POST, &Private::setPassword, this
    },
    httpd_uri_t{
      "/config/version", HTTP_GET, &Private::getVersion, this
    }
  };

  static std::optional<std::string> readBody(httpd_req_t* req) {
    std::string body;
    std::array<char, 32> buffer;
    int read = 0;
    int remaining = req->content_len;

    while (remaining > 0) {
      read = httpd_req_recv(req, buffer.data(), std::min(remaining, static_cast<int>(buffer.size())));
      if (read <= 0) {
        if (read == HTTPD_SOCK_ERR_TIMEOUT) {
          continue;
        }
        return std::nullopt;
      }
      remaining -= read;
      body += std::string(buffer.data(), buffer.data() + read);
    }
    return body;
  }

  static esp_err_t getReset(httpd_req_t* req) {
    httpd_resp_set_status(req, "302 Found");
    httpd_resp_set_hdr(req, "Location", "/");
    httpd_resp_send(req, nullptr, 0);
    xTaskCreate(&Private::restartTask, "restartTask", 2048, nullptr, tskIDLE_PRIORITY, nullptr);
    return ESP_OK;
  }

  static void restartTask(void* params) {
    vTaskDelay(pdMS_TO_TICKS(2000));
    esp_restart();
  }

  static esp_err_t getIndex(httpd_req_t* req) {
    httpd_resp_set_hdr(req, "Content-Encoding", "gzip");
    httpd_resp_send(req, reinterpret_cast<const char*>(indexHtmlBegin), indexHtmlEnd - indexHtmlBegin);
    return ESP_OK;
  }

  static esp_err_t getAppJs(httpd_req_t* req) {
    httpd_resp_set_hdr(req, "Content-Encoding", "gzip");
    httpd_resp_send(req, reinterpret_cast<const char*>(appJsBegin), appJsEnd - appJsBegin);
    return ESP_OK;
  }

  static esp_err_t getSsid(httpd_req_t* req) {
    Private* p = static_cast<Private*>(req->user_ctx);
    std::string ssid = p->ssid;
    httpd_resp_send(req, ssid.c_str(), ssid.size());
    return ESP_OK;
  }

  // TODO refactor these little functions into one single form handler
  static esp_err_t setSsid(httpd_req_t* req) {
    Private* p = static_cast<Private*>(req->user_ctx);
    std::optional<std::string> newSsid = readBody(req);
    if (!newSsid) {
      httpd_resp_send_404(req);
      return ESP_OK;
    }
    
    p->ssid = *newSsid;
    httpd_resp_send(req, newSsid->c_str(), newSsid->size());
    return ESP_OK;
  }

  static esp_err_t getPassword(httpd_req_t* req) {
    Private* p = static_cast<Private*>(req->user_ctx);
    std::string password = p->password;
    httpd_resp_send(req, password.c_str(), password.size());
    return ESP_OK;
  }
  
  static esp_err_t setPassword(httpd_req_t* req) {
    Private* p = static_cast<Private*>(req->user_ctx);
    std::optional<std::string> newPassword = readBody(req);
    if (!newPassword) {
      httpd_resp_send_404(req);
      return ESP_OK;
    }
    
    p->password = *newPassword;
    httpd_resp_send(req, newPassword->c_str(), newPassword->size());
    return ESP_OK;
  }

  static esp_err_t getVersion(httpd_req_t* req) {
    std::string versionString = std::string(version);
    httpd_resp_send(req, versionString.c_str(), versionString.size());
    return ESP_OK;
  }

  Private(Config::Value<std::string>& ssid, Config::Value<std::string>& password) 
    : ssid(ssid), password(password) {
    initializeWifi();
  }

  void initializeWifi() {
    ESP_LOGI(TAG_WIFI, "initializing wifi");

    tcpip_adapter_init();
    auto error = esp_event_loop_create_default();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    error |= esp_wifi_init(&cfg);

    error |= esp_event_handler_register(
      WIFI_EVENT, 
      ESP_EVENT_ANY_ID, 
      &Private::stationEventHandler, 
      this
    );
    error |= esp_event_handler_register(
      IP_EVENT, 
      IP_EVENT_STA_GOT_IP, 
      &Private::stationEventHandler, 
      this
    );

    wifi_config_t wifiConfig{};
    std::string rawSsid = ssid;
    std::memcpy(wifiConfig.sta.ssid, rawSsid.data(), rawSsid.size());

    std::string rawPass = password;
    std::memcpy(wifiConfig.sta.password, rawPass.data(), rawPass.size());

    wifiConfig.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    wifiConfig.sta.pmf_cfg.capable = true;
    wifiConfig.sta.pmf_cfg.required = false;

    error |= esp_wifi_set_mode(WIFI_MODE_STA);
    error |= esp_wifi_set_config(ESP_IF_WIFI_STA, &wifiConfig);
    error |= esp_wifi_start();
    ESP_ERROR_CHECK(error);

    ESP_LOGI(
      TAG_WIFI, 
      "trying to connect to AP SSID: '%s', password: '%s'...",
      rawSsid.c_str(), 
      rawPass.c_str()
    );

    EventBits_t bits = xEventGroupWaitBits(
      wifiEvents,
      WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
      pdFALSE,
      pdFALSE,
      pdMS_TO_TICKS(15000)
    );

    if (bits & WIFI_CONNECTED_BIT) {
      ESP_LOGI(
        TAG_WIFI, 
        "connected to ap SSID: '%s', password: '%s'",
        rawSsid.c_str(), 
        rawPass.c_str()
      );
      isConnected = true;
    } else if (bits & WIFI_FAIL_BIT) {
      ESP_LOGE(
        TAG_WIFI, 
        "failed to connect to SSID: '%s', password: '%s'",
        rawSsid.c_str(), 
        rawPass.c_str()
      );
      isConnected = false;
    } else {
      ESP_LOGW(TAG_WIFI, "wifi connection timed out, setting up fallback WiFi AP");
      isConnected = false;
    }

    if (!isConnected) {
      startAccessPoint();
      startSettingsServer();
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
      ESP_LOGI(TAG_WIFI, "retry to connect to the AP");
    } else if (eventBase == IP_EVENT && eventId == IP_EVENT_STA_GOT_IP) {
      ip_event_got_ip_t* event = (ip_event_got_ip_t*)eventData;
      ESP_LOGI(TAG_WIFI, "got ip: %s", ip4addr_ntoa(&event->ip_info.ip));
      xEventGroupSetBits(p->wifiEvents, WIFI_CONNECTED_BIT);
    }
  }

  void startAccessPoint() {
    auto error = esp_event_handler_unregister(
      WIFI_EVENT, 
      ESP_EVENT_ANY_ID, 
      &Private::stationEventHandler
    );

    error |= esp_event_handler_unregister(
      IP_EVENT, 
      IP_EVENT_STA_GOT_IP, 
      &Private::stationEventHandler
    );

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    error |= esp_wifi_init(&cfg);

    error |= esp_event_handler_register(
      WIFI_EVENT, 
      ESP_EVENT_ANY_ID, 
      &Private::apEventHandler, 
      this
    );
    
    ESP_LOGI(
      TAG_WIFI, 
      "initializing AP SSID: '%s', password: '%s', channel: '%d'",
      std::string(DEFAULT_AP_SSID).c_str(), 
      std::string(DEFAULT_AP_PASS).c_str(), 
      DEFAULT_AP_CHANNEL
    );

    wifi_config_t wifiConfig{};
    
    std::memcpy(wifiConfig.ap.ssid, DEFAULT_AP_SSID.data(), DEFAULT_AP_SSID.size());
    std::memcpy(wifiConfig.ap.password, DEFAULT_AP_PASS.data(), DEFAULT_AP_PASS.size());
    wifiConfig.ap.ssid_len = DEFAULT_AP_SSID.size();
    wifiConfig.ap.channel = DEFAULT_AP_CHANNEL;
    wifiConfig.ap.max_connection = 4;
    wifiConfig.ap.authmode = WIFI_AUTH_WPA_WPA2_PSK;

    error |= esp_wifi_set_mode(WIFI_MODE_AP);
    error |= esp_wifi_set_config(ESP_IF_WIFI_AP, &wifiConfig);
    error |= esp_wifi_start();
    ESP_ERROR_CHECK(error);
    
    isConnected = true;
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

  void startSettingsServer() {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    ESP_LOGI(TAG_WIFI, "Starting settings server on port %d", config.server_port);
    ESP_ERROR_CHECK(httpd_start(&server, &config));

    ESP_LOGI(TAG_WIFI, "Registering URI handlers");
    for (const auto& handler : handlerDefinitions) {
      httpd_register_uri_handler(server, &handler);
    }
  }

  ~Private() {
    httpd_stop(server);

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
  }
};

Wifi::Wifi(Config::Value<std::string>& ssid, Config::Value<std::string>& password) 
  : p(std::make_unique<Private>(ssid, password)) { }

Wifi::~Wifi() = default;

}
