#pragma once

#include <array>
#include <charconv>
#include <chrono>
#include <cmath>
#include <functional>
#include <limits>
#include <memory>
#include <optional>
#include <string_view>
#include <type_traits>

namespace essentials {

struct Mqtt {
  static constexpr std::string_view FALSE_LITERAL = "false";
  static constexpr std::string_view TRUE_LITERAL = "true";
  static constexpr std::string_view NAN_LITERAL = "NaN";
  static constexpr std::string_view POSITIVE_INF_LITERAL = "Infinity";
  static constexpr std::string_view NEGATIVE_INF_LITERAL = "-Infinity";

  struct ConnectionInfo {
    std::string_view uri;
    std::string_view cert;
    std::string_view username;
    std::string_view password;
  };

  enum class Qos : int { Qos0, Qos1, Qos2 };

  struct Data {
    std::string_view data;
    int32_t offset;
    int32_t totalLength;
  };

  struct Subscription {
    std::string_view topic;
    Qos qos;

    ~Subscription() {
      _unsubscribe();
    }

  private:
    friend struct Mqtt;
    std::function<void(const Data&)> _reaction;
    std::function<void()> _unsubscribe;
  };

  struct LastWillMessage {
    std::string topic;
    std::string message;
    Qos qos;
    bool isRetained;
  };

  // TODO separate connection from constructor
  /**
   * @brief Create Mqtt and connect to a server
   *
   * @param connectionInfo
   * @param topicsPrefix topic's prefix for subscribe and publish operations
   * @param keepAlive
   * @param lastWillMessage LWT MQTT message
   * @param onConnect on connect callback
   * @param onDisconnect on disconnect callback
   * @param bufferSize buffer size for MQTT data. Incoming messages bigger than bufferSize will result in calling
   * subscribe's reaction multiple times. For bigger data sizes use reaction function with parameter const Mqtt::Data&
   * which tells you info about incoming data chunk (offset, total length).
   */
  Mqtt(ConnectionInfo connectionInfo,
    std::string_view topicsPrefix,
    std::chrono::seconds keepAlive = std::chrono::seconds{120},
    std::optional<LastWillMessage> lastWillMessage = std::nullopt,
    std::function<void()> onConnect = nullptr,
    std::function<void()> onDisconnect = nullptr,
    int32_t bufferSize = 1024);
  ~Mqtt();

  bool isConnected() const;

  // TODO implement subscription to multi-level and single-level wildcard topics (eg. 'example/#',
  // 'example/+/temperature')
  /**
   * @brief Subscribe to a given MQTT topic with a callback
   *
   * @param topic MQTT topic to subscribe. Topic's prefix is prepended.
   * @param qos MQTT qos
   * @param reaction callback function. Callback parameter contains info about incoming data chunk. If buffer is not big
   * enough, this callback is called multiple times.
   * @return std::unique_ptr<Subscription> delete of subscription results in MQTT unsubscribe
   */
  std::unique_ptr<Subscription> subscribe(std::string_view topic, Qos qos, std::function<void(const Data&)> reaction);

  /**
   * @brief Subscribe to a given MQTT topic with a callback
   *
   * @param topic MQTT topic to subscribe. Topic's prefix is prepended.
   * @param qos MQTT qos
   * @param reaction callback function. Callback parameter contains data and if buffer is not big
   * enough, this callback is called multiple times thus data will be fragmented.
   * @return std::unique_ptr<Subscription> delete of subscription results in MQTT unsubscribe
   */
  std::unique_ptr<Subscription> subscribe(
    std::string_view topic, Qos qos, std::function<void(std::string_view)> reaction);

  /**
   * @brief Subscribe to a given MQTT topic with a callback with value conversion
   *
   * @tparam T type for value conversion (supported types are bool, integral types, floating point types). Bool type
   * expects messages with a value "false" (FALSE_LITERAL), "true" (TRUE_LITERAL) on topic.
   * @param topic MQTT topic to subscribe. Topic's prefix is prepended.
   * @param qos MQTT qos
   * @param reaction callback function. Callback parameter contains converted data into given T type
   * @return std::unique_ptr<Subscription> delete of subscription results in MQTT unsubscribe
   */
  template<typename T, typename std::enable_if_t<!std::is_constructible_v<std::string_view, T>>* = nullptr>
  std::unique_ptr<Subscription> subscribe(
    std::string_view topic, Qos qos, std::function<void(std::optional<T>)> reaction) {
    return subscribe(topic, qos, [reaction](std::string_view data) { reaction(_fromString<T>(data)); });
  }

  /**
   * @brief Subscribe to a given MQTT topic with a value reference. Value is changed automatically when new message
   * arrive.
   *
   * @tparam T type for value conversion (supported types are bool, integral types, floating point types). Bool type
   * expects messages with a value "false" (FALSE_LITERAL), "true" (TRUE_LITERAL) on topic.
   * @param topic MQTT topic to subscribe. Topic's prefix is prepended.
   * @param qos MQTT qos
   * @param value reference to a value where incoming message will be stored
   * @return std::unique_ptr<Subscription> delete of subscription results in MQTT unsubscribe
   */
  template<typename T>
  std::unique_ptr<Subscription> subscribe(std::string_view topic, Qos qos, T& value) {
    return subscribe(topic, qos, [&value](std::string_view data) {
      if constexpr (std::is_same_v<T, std::string>) {
        value = std::string(data);
      } else {
        std::optional<T> incomingValue = _fromString<T>(data);
        if (!incomingValue) return;

        value = *incomingValue;
      }
    });
  }

  /**
   * @brief Publish MQTT message
   *
   * @param topic
   * @param data
   * @param qos
   * @param isRetained
   */
  void publish(std::string_view topic, std::string_view data, Qos qos, bool isRetained);

  /**
   * @brief Publish MQTT message
   *
   * @tparam T
   * @param topic
   * @param value
   * @param qos
   * @param isRetained
   */
  template<typename T, typename std::enable_if_t<!std::is_constructible_v<std::string_view, T>>* = nullptr>
  void publish(std::string_view topic, T value, Qos qos, bool isRetained) {
    publish(topic, _toString(value), qos, isRetained);
  }

private:
  struct Private;
  std::unique_ptr<Private> p;

  static constexpr size_t MAX_DIGITS = 64;

  template<typename T>
  static std::string _toString(T value) {
    constexpr bool isValidType = std::is_same_v<T, bool> || std::is_integral_v<T> || std::is_floating_point_v<T>;
    static_assert(isValidType, "T must be bool, integral or floating point");

    if constexpr (std::is_same_v<T, bool>) {
      return value ? std::string(TRUE_LITERAL) : std::string(FALSE_LITERAL);
    } else if constexpr (std::is_integral_v<T>) {
      std::array<char, MAX_DIGITS> buffer;
      auto [p, ec] = std::to_chars(buffer.data(), buffer.data() + buffer.size(), value);
      if (ec != std::errc()) return std::string(NAN_LITERAL);

      return std::string{buffer.begin(), std::size_t(p - buffer.begin())};
    } else if constexpr (std::is_floating_point_v<T>) {
      if (std::isnan(value)) return std::string(NAN_LITERAL);

      if (std::isinf(value)) return std::string(value > 0.0 ? POSITIVE_INF_LITERAL : NEGATIVE_INF_LITERAL);

      // TODO use std::to_chars when will be implemented in GCC for floating point types
      return std::to_string(value);
    }
    return "";
  }

  template<typename T>
  static std::optional<T> _fromString(std::string_view textValue) {
    constexpr bool isValidType = std::is_same_v<T, bool> || std::is_integral_v<T> || std::is_floating_point_v<T>;
    static_assert(isValidType, "T must be bool, integral or floating point");

    if constexpr (std::is_same_v<T, bool>) {
      if (textValue == TRUE_LITERAL) return true;
      else if (textValue == FALSE_LITERAL)
        return false;
      return std::nullopt;
    } else if constexpr (std::is_integral_v<T>) {
      std::optional<T> value;
      T rawValue;
      auto [_, ec] = std::from_chars(textValue.begin(), textValue.end(), rawValue);
      if (ec == std::errc()) {
        value = rawValue;
      }
      return value;
    } else if constexpr (std::is_floating_point_v<T>) {
      std::optional<T> value;
      if (textValue == NAN_LITERAL) return std::numeric_limits<T>::quiet_NaN();

      if (textValue == POSITIVE_INF_LITERAL) return std::numeric_limits<T>::infinity();

      if (textValue == NEGATIVE_INF_LITERAL) return -std::numeric_limits<T>::infinity();

      // TODO use std::from_chars when will be implemented in GCC for floating point types
      try {
        if constexpr (std::is_same_v<T, float>) {
          return std::stof(std::string(textValue));
        }
        if constexpr (std::is_same_v<T, double>) {
          return std::stod(std::string(textValue));
        }
        if constexpr (std::is_same_v<T, long double>) {
          return std::stold(std::string(textValue));
        }
      } catch (const std::exception&) {
        return std::nullopt;
      }
    }
  }
};

}
