#include <algorithm>

#include <bitset>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <istream>
#include <iterator>
#include <print>
#include <ranges>
#include <span>
#include <sys/types.h>
#include <type_traits>
#include <utility>

constexpr bool DEBUG_MODE = false;

template <typename T>
concept ByteType = sizeof(T) == 1 && std::is_trivial_v<T>;

template <typename T>
concept ByteContiguousRange =
    std::ranges::contiguous_range<T> && std::ranges::sized_range<T> &&
    ByteType<std::ranges::range_value_t<T>>;

template <typename T>
concept ByteInputStream =
    std::derived_from<std::remove_cvref_t<T>, std::istream>;

auto generate_bytes_view(auto &&input) {
  using T = decltype(input);

  if constexpr (ByteContiguousRange<T>) {
    return std::as_bytes(std::span{input});
  } else if constexpr (ByteInputStream<T>) {
    return std::ranges::subrange(std::istreambuf_iterator<char>(input),
                                 std::istreambuf_iterator<char>()) |
           std::views::transform(
               [](auto &&x) { return static_cast<std::byte>(x); });
  } else {
    static_assert(ByteContiguousRange<T> || ByteInputStream<T>,
                  "Input must be either a contiguous byte container or "
                  "an input stream.");
  }
}

uint8_t xor_checksum(auto &&input) {
  return std::to_integer<uint8_t>(std::ranges::fold_left(
      generate_bytes_view(input), std::byte{0x00}, std::bit_xor<>()));
}

std::pair<uint8_t, uint8_t> fletcher_checksum(auto &&input) {
  using ReturnType = std::pair<uint8_t, uint8_t>;

  ReturnType result{0, 0};
  auto fletcher = [&result](uint8_t x) {
    int a = (static_cast<uint16_t>(result.first) + x) % 0xFF,
        b = (static_cast<uint16_t>(result.second) + a) % 0xFF;

    result.first = a;
    result.second = b;
  };

  auto uint8_view =
      generate_bytes_view(input) | std::views::transform([](std::byte b) {
        return std::to_integer<uint8_t>(b);
      });
  std::ranges::for_each(uint8_view, fletcher);
  return result;
}

template <typename ResultType, size_t width>
ResultType crc_checksum(auto &&input, ResultType poly, ResultType init_value,
                        ResultType xor_out, bool reflect_in, bool reflect_out) {

  static_assert(sizeof(ResultType) * 8 >= width);

  auto reflect = [](std::byte b) -> std::byte {
    std::byte result{0x00};
    for (uint8_t i = 0; i != 8; ++i) {
      result |= ((b >> i) & std::byte{0x01}) << (7 - i);
    }
    return result;
  };

  auto bytes_view =
      generate_bytes_view(input) |
      std::views::transform([&reflect_in, &reflect](auto &&value) {
        /*
        CRC needs to shift in MSB, but little endian is
        default in my PC so raw data is already reflect, we
        will reflect when reflect_in is false
         */
        if (!reflect_in)
          return reflect(value);
        else
          return value;
      });

  constexpr auto header_size = (width + 7) / 8;
  // header should xor with init value
  auto head_bytes_view =
      bytes_view | std::views::take(header_size) | std::views::enumerate |
      std::views::transform([&init_value, &header_size](auto &&pair) {
        auto [index, value] = pair;
        return value ^
               std::byte{static_cast<uint8_t>(
                   (init_value >> ((header_size - index - 1) * 8)) & 0xFF)};
      });

  auto tail_bytes_view = bytes_view | std::views::drop(header_size);

  std::bitset<width> result{0x00};

  auto work = [&result, &poly]<size_t N>(const std::bitset<N> &data) {
    for (uint8_t i = 0; i != data.size(); ++i) {
      bool move_out = result.test(result.size() - 1);
      // move in new bit
      result <<= 1;
      if (data.test(i))
        result.set(0);
      else
        result.reset(0);

      // DEBUG MODE
      if constexpr (DEBUG_MODE)
        std::print("before: {:02X} ", result.to_ullong());

      // xor with poly
      if (move_out) {
        result ^= std::bitset<width>{poly};
      }

      // DEBUG MODE
      if constexpr (DEBUG_MODE)
        std::print("after: {:02X}\n", result.to_ullong());
    }
  };
  // deal with input data
  std::ranges::for_each(head_bytes_view, [&work](const std::byte b) {
    work(std::bitset<8>{std::to_integer<uint8_t>(b)});
  });
  std::ranges::for_each(tail_bytes_view, [&work](const std::byte b) {
    work(std::bitset<8>{std::to_integer<uint8_t>(b)});
  });
  // deal with complement zeros
  work(std::bitset<width>{0x00});

  if (reflect_out) {
    auto result_copy = result;
    for (int i = 0; i != result.size(); ++i) {
      result_copy.test(i) ? result.set(result.size() - 1 - i)
                          : result.reset(result.size() - 1 - i);
    }
  }
  return result.to_ullong() ^ xor_out;
}
