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

  struct Subscription {
    std::string_view topic;
    Qos qos;

    ~Subscription() {
      _unsubscribe();
    }

  private:
    friend struct Mqtt;
    std::function<void(std::string_view)> _reaction;
    std::function<void()> _unsubscribe;
  };

  struct LastWillMessage {
    std::string topic;
    std::string message;
    Qos qos;
    bool isRetained;
  };

  // TODO separate connection from constructor
  Mqtt(ConnectionInfo connectionInfo,
    std::string_view topicsPrefix,
    std::chrono::seconds keepAlive = std::chrono::seconds{120},
    std::optional<LastWillMessage> lastWillMessage = std::nullopt,
    std::function<void()> onConnect = nullptr,
    std::function<void()> onDisconnect = nullptr);
  ~Mqtt();

  bool isConnected() const;

  // TODO implement subscription to multi-level and single-level wildcard topics (eg. 'example/#',
  // 'example/+/temperature')
  std::unique_ptr<Subscription> subscribe(
    std::string_view topic, Qos qos, std::function<void(std::string_view)> reaction);
  void publish(std::string_view topic, std::string_view data, Qos qos, bool isRetained);

  template<typename T, typename std::enable_if_t<!std::is_constructible_v<std::string_view, T>>* = nullptr>
  std::unique_ptr<Subscription> subscribe(
    std::string_view topic, Qos qos, std::function<void(std::optional<T>)> reaction) {
    return subscribe(topic, qos, [reaction](std::string_view data) { reaction(_fromString<T>(data)); });
  }

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
