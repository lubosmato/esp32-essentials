#pragma once

namespace essentials {

// TODO replace with std::span when esp-idf will use toolchain with std::span
template<typename T>
struct Span {
  const T* data;
  std::size_t size;
};

}
