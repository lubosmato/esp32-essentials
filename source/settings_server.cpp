#include "essentials/settings_server.hpp"

#include "cJSON.h"
#include "esp_http_server.h"
#include "esp_log.h"

#include <array>
#include <optional>
#include <string>

namespace essentials {

const char* TAG_SETTINGS_SERVER = "settings_server";

extern const uint8_t indexHtmlBegin[] asm("_binary_index_html_gz_start");
extern const uint8_t indexHtmlEnd[] asm("_binary_index_html_gz_end");

extern const uint8_t appJsBegin[] asm("_binary_app_js_gz_start");
extern const uint8_t appJsEnd[] asm("_binary_app_js_gz_end");

struct SettingsServer::Private {
  uint16_t port{};
  httpd_handle_t server = nullptr;
  bool isRunning = false;
  std::vector<Field> fields{};
  std::string deviceName{};
  std::string version{};

  std::array<httpd_uri_t, 4> handlerDefinitions{
    httpd_uri_t{
      "/", HTTP_GET, &Private::getIndex, this
    },
    httpd_uri_t{
      "/app.js", HTTP_GET, &Private::getAppJs, this
    },
    httpd_uri_t{
      "/settings", HTTP_GET, &Private::getSettings, this
    },
    
    httpd_uri_t{
      "/settings", HTTP_POST, &Private::setSettings, this
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

  static esp_err_t getSettings(httpd_req_t* req) {
    Private* p = static_cast<Private*>(req->user_ctx);
    std::string settingsJson = p->makeSettingsJson();
    httpd_resp_set_hdr(req, "Content-Type", "application/json");
    httpd_resp_send(req, settingsJson.data(), settingsJson.size());
    return ESP_OK;
  }

  std::string makeSettingsJson() {
    cJSON* json = cJSON_CreateObject();
    for (auto& field : fields) {
      cJSON_AddStringToObject(json, field.label.c_str(), field.value->c_str());
    }
    cJSON_AddStringToObject(json, "deviceName", deviceName.c_str());
    cJSON_AddStringToObject(json, "version", version.c_str());

    std::unique_ptr<char[]> rawData{cJSON_PrintUnformatted(json)};
    cJSON_free(json);
    return std::string{rawData.get()};
  }

  static esp_err_t setSettings(httpd_req_t* req) {
    Private* p = static_cast<Private*>(req->user_ctx);
    std::optional<std::string> postData = readBody(req);
    if (postData) { 
      p->setSettingsFromJson(std::move(*postData));
    }
    httpd_resp_set_status(req, "302 Found");
    httpd_resp_set_hdr(req, "Location", "/");
    httpd_resp_send(req, nullptr, 0);

    ESP_LOGI(TAG_SETTINGS_SERVER, "Accepted new settings, restarting...");

    xTaskCreate(
      [](void*) {
        vTaskDelay(pdMS_TO_TICKS(2000));
        esp_restart();
      }, 
      "restartTask", 
      1024, 
      nullptr, 
      tskIDLE_PRIORITY, 
      nullptr
    );

    return ESP_OK;
  }

  void setSettingsFromJson(std::string jsonContent) {
    cJSON* json = cJSON_Parse(jsonContent.c_str());
    
    for (auto& field : fields) {
      field.value = cJSON_GetObjectItem(json, field.label.c_str())->valuestring;
    }
    cJSON_free(json);
  }

  static esp_err_t getIndex(httpd_req_t* req) {
    httpd_resp_set_hdr(req, "Content-Encoding", "gzip");
    httpd_resp_set_hdr(req, "Cache-Control", "public, max-age=2419200");
    httpd_resp_send(req, reinterpret_cast<const char*>(indexHtmlBegin), indexHtmlEnd - indexHtmlBegin);
    return ESP_OK;
  }

  static esp_err_t getAppJs(httpd_req_t* req) {
    httpd_resp_set_hdr(req, "Content-Encoding", "gzip");
    httpd_resp_set_hdr(req, "Cache-Control", "public, max-age=2419200");
    httpd_resp_send(req, reinterpret_cast<const char*>(appJsBegin), appJsEnd - appJsBegin);
    return ESP_OK;
  }

  void start() {
    if (isRunning) return;

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = port;

    ESP_LOGI(TAG_SETTINGS_SERVER, "Starting settings server on port %d", config.server_port);
    ESP_ERROR_CHECK(httpd_start(&server, &config));

    ESP_LOGI(TAG_SETTINGS_SERVER, "Registering URI handlers");
    for (const auto& handler : handlerDefinitions) {
      httpd_register_uri_handler(server, &handler);
    }
    isRunning = true;
  }

  void stop() {
    if (!isRunning) return;
    httpd_stop(server);
    server = nullptr;
    isRunning = false;
  }

  ~Private() {
    stop();
  }

};

SettingsServer::SettingsServer(uint16_t port, std::string_view deviceName, std::string_view version, std::vector<Field> fields) 
  : p(std::make_unique<Private>()) {
  p->port = port;
  p->deviceName = deviceName;
  p->version = version;
  p->fields = std::move(fields);
}

SettingsServer::~SettingsServer() = default;

void SettingsServer::start() {
  p->start();
}

void SettingsServer::stop() {
  p->stop();
}

}
