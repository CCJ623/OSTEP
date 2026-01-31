
module;
#include <algorithm>
#include <array>
#include <bitset>
#include <chrono>
#include <concepts>
#include <cstddef>
#include <cstring>
#include <expected>
#include <iterator>
#include <print>
#include <ranges>
#include <span>
#include <string_view>
#include <system_error>
#include <thread>
#include <utility>
#include <vector>

#include "udp.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <sys/socket.h>

module Communication;

class RetransmissionTimer {
  using Clock = std::chrono::steady_clock;
  Clock::time_point last_sent_time_;
  std::chrono::milliseconds timeout_threshold_;

public:
  explicit RetransmissionTimer(size_t ms) : timeout_threshold_(ms) {
    restart();
  }

  void restart() { last_sent_time_ = Clock::now(); }

  [[nodiscard]] bool isExpired() const {
    return (Clock::now() - last_sent_time_) >= timeout_threshold_;
  }

  auto elapsedMs() const {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               Clock::now() - last_sent_time_)
        .count();
  }
};

template <std::unsigned_integral UI>
SequenceNumberType mapToSequenceNumber(UI number) {
  return number % REAL_DATA_SEQUENCE_NUMBER_RANGE.size();
}

size_t mapToIndex(SequenceNumberType sequence_number, size_t start) {
  auto base = start / REAL_DATA_SEQUENCE_NUMBER_RANGE.size() *
              REAL_DATA_SEQUENCE_NUMBER_RANGE.size();
  return base + sequence_number;
}

template <std::ranges::contiguous_range R>
std::string_view interpretAsString(const R &data) {
  return std::string_view{
      reinterpret_cast<const char *>(std::ranges::data(data)),
      std::ranges::size(data)};
}

Socket::Socket(int port) : socket_(UDP_Open(port)) {
  if (socket_ == -1) {
    throw std::runtime_error{"open socket error"};
  }
}

Socket::~Socket() { UDP_Close(socket_); }

template <std::ranges::contiguous_range R>
  requires(sizeof(std::ranges::range_value_t<R>) == 1)
SequenceNumberType getSequenceNumber(const R &segment) {
  SequenceNumberType result;
  std::memcpy(&result, segment.data(), SEQUENCE_NUMBER_SIZE);
  return result;
}

template <std::ranges::contiguous_range R> auto getMessage(R &&segment) {
  return segment | std::ranges::views::drop(SEQUENCE_NUMBER_SIZE);
}

bool isControlSegment(const SequenceNumberType &sequence_number) {
  return sequence_number == MAX_SEQUENCE_NUMBER;
}

bool isControlSegment(const std::span<const char> segment) {
  return isControlSegment(getSequenceNumber(segment));
}

bool isAckSegment(const std::span<const char> segment) {
  return !isControlSegment(segment) &&
         interpretAsString(getMessage(segment)) == ACK_MESSAGE;
}

template <std::ranges::contiguous_range R>
  requires(sizeof(std::ranges::range_value_t<R>) == 1)
inline std::vector<std::byte>
generateSegment(SequenceNumberType sequence_number, const R &data) {

  std::vector<std::byte> result;
  result.reserve(SEQUENCE_NUMBER_SIZE + data.size());
  auto sequence_number_ptr = reinterpret_cast<std::byte *>(&sequence_number);
  auto sequence_number_view = std::as_bytes(std::span{&sequence_number, 1});
  result.append_range(sequence_number_view);
  auto data_view = std::as_bytes(std::span{data});
  result.append_range(data_view);

  return result;
}

auto generateControlSegment(ControlSegmentType type) {
  return generateSegment(MAX_SEQUENCE_NUMBER,
                         std::as_bytes(std::span{&type, sizeof(type)}));
}

ControlSegmentType
getControlSegmentType(const std::span<const std::byte> segment) {
  ControlSegmentType result;
  std::memcpy(&result, getMessage(segment).data(), sizeof(ControlSegmentType));
  return result;
}

ControlSegmentType Socket::getControlSegmentType() const {
  ControlSegmentType result;
  std::memcpy(&result, message_views_[MAX_SEQUENCE_NUMBER].data(),
              sizeof(ControlSegmentType));
  return result;
}

// [operation][ip:port][sequence number][message]
void printLog(std::string_view operation, sockaddr_in &addr,
              SequenceNumberType sequence_number, std::string_view message) {

  constexpr size_t message_limit = 15;
  if (message.size() > message_limit) {
    std::println(
        "[thread:{}][{}][{}:{}][{}][{}...][{}B]", std::this_thread::get_id(),
        operation, inet_ntoa(addr.sin_addr), ntohs(addr.sin_port),
        sequence_number, message.substr(0, message_limit), message.size());
  } else {
    std::println("[thread:{}][{}][{}:{}][{}][{}][{}B]",
                 std::this_thread::get_id(), operation,
                 inet_ntoa(addr.sin_addr), ntohs(addr.sin_port),
                 sequence_number, message, message.size());
  }
}

template <std::integral I>
void printLog(std::string_view operation, sockaddr_in &addr,
              SequenceNumberType sequence_number, const I &integer) {
  std::println("[thread:{}][{}][{}:{}][{}][{:x}][{}B]",
               std::this_thread::get_id(), operation, inet_ntoa(addr.sin_addr),
               ntohs(addr.sin_port), sequence_number, integer, sizeof(I));
}

void printLog(std::string_view operation, sockaddr_in &addr,
              const std::span<const std::byte> segment) {
  auto sequence_number = getSequenceNumber(segment);
  if (isControlSegment(sequence_number)) {
    printLog(operation, addr, sequence_number,
             std::to_underlying(::getControlSegmentType(segment)));

  } else {
    printLog(operation, addr, sequence_number,
             interpretAsString(getMessage(segment)));
  }
}

/*
return value: sequence number
*/
template <ReadMode Mode = ReadMode::Blocking>
std::expected<SequenceNumberType, std::errc>
Socket::readFromSocket(sockaddr_in &addr) {
  std::array<char, MAXIMUN_TRANSMISSION_UNIT> temp_buffer;
  ssize_t num_read_bytes = 0;

  if constexpr (Mode == ReadMode::Blocking) {
    num_read_bytes =
        UDP_Read(socket_, &addr, temp_buffer.data(), temp_buffer.size());
  } else {
    socklen_t addr_len = sizeof(addr);
    num_read_bytes =
        recvfrom(socket_, temp_buffer.data(), temp_buffer.size(), MSG_DONTWAIT,
                 (struct sockaddr *)&addr, &addr_len);
  }

  if (num_read_bytes < 0) {
    if (errno == EAGAIN || errno == EWOULDBLOCK) {
      return std::unexpected(std::errc::resource_unavailable_try_again);
    }
    return std::unexpected(static_cast<std::errc>(errno));
  }

  if (num_read_bytes < SEQUENCE_NUMBER_SIZE) {
    return std::unexpected(std::errc::bad_message);
  }

  auto received_segment =
      temp_buffer | std::ranges::views::take(num_read_bytes);
  auto sequence_number = getSequenceNumber(received_segment);
  std::ranges::copy(getMessage(temp_buffer),
                    buffer_.at(sequence_number).begin());
  message_views_[sequence_number] = std::span{
      buffer_[sequence_number] |
      std::ranges::views::take(num_read_bytes - SEQUENCE_NUMBER_SIZE)};

  if constexpr (DEBUG_MODE) {
    printLog("receive", addr, std::as_bytes(std::span{received_segment}));
  };
  return sequence_number;
}

template <std::ranges::contiguous_range R>
void Socket::writeToSocket(sockaddr_in &addr,
                           SequenceNumberType sequence_number, const R &data) {

  auto send_segment = generateSegment(sequence_number, data);
  UDP_Write(socket_, &addr, reinterpret_cast<const char *>(send_segment.data()),
            static_cast<int>(send_segment.size()));
  if constexpr (DEBUG_MODE) {
    printLog("send", addr, send_segment);
  };
}

void Socket::writeToSocket(sockaddr_in &addr,
                           const std::span<const std::byte> segment) {

  UDP_Write(socket_, &addr, reinterpret_cast<const char *>(segment.data()),
            static_cast<int>(segment.size()));
  if constexpr (DEBUG_MODE) {
    auto sequence_number = getSequenceNumber(segment);
    printLog("send", addr, segment);
  };
}

std::vector<char> Socket::read(sockaddr_in *addr) {

  SequenceNumberType max_sequence_number = 0;
  // total bytes of whole message, excluding sequence number
  size_t total_bytes = 0;
  std::bitset<std::ranges::size(REAL_DATA_SEQUENCE_NUMBER_RANGE)> is_received;
  size_t receive_base_index = 0;
  size_t next_to_receive_index = 0;
  RetransmissionTimer retransmission_timer(500);
  std::vector<char> return_data;

  auto getBufferRange = [&]() {
    return std::views::iota(receive_base_index, next_to_receive_index);
  };

  auto getAvailableRange = [&]() {
    return std::views::iota(next_to_receive_index,
                            receive_base_index + SEND_WINDOW_SIZE);
  };

  auto isAvailableSequenceNumber = [&](SequenceNumberType sequence_number) {
    auto index = mapToIndex(sequence_number, receive_base_index);
    return receive_base_index <= index &&
           index < receive_base_index + RECEIVE_WINDOW_SIZE;
  };

  // handshake
  [&]() {
    while (true) {
      auto result = readFromSocket<ReadMode::Blocking>(*addr);

      if (!result) {
        throw std::system_error(make_error_code(result.error()));
      }

      auto sequence_number = *result;
      if (isControlSegment(sequence_number)) {
        switch (getControlSegmentType()) {
        case ControlSegmentType::HANDSHAKE:
          writeToSocket(*addr,
                        generateControlSegment(ControlSegmentType::HANDSHAKE));
          return;
          break;
        case ControlSegmentType::FINISH:
          writeToSocket(*addr,
                        generateControlSegment(ControlSegmentType::FINISH));
          break;
        default:
          break;
        }
      } else {
        writeToSocket(*addr, sequence_number, ACK_MESSAGE);
      }
    }
  }();

  // receive data
  [&]() {
    while (true) {
      auto result = readFromSocket<ReadMode::Blocking>(*addr);

      if (!result) {
        throw std::system_error(make_error_code(result.error()));
      }

      auto sequence_number = *result;
      if (isControlSegment(sequence_number)) {
        switch (getControlSegmentType()) {
        case ControlSegmentType::HANDSHAKE:
          writeToSocket(*addr,
                        generateControlSegment(ControlSegmentType::HANDSHAKE));
          continue;
          break;
        case ControlSegmentType::FINISH:
          writeToSocket(*addr,
                        generateControlSegment(ControlSegmentType::FINISH));
          return;
          break;
        default:
          break;
        }
      } else if (isAvailableSequenceNumber(sequence_number)) {
        next_to_receive_index =
            std::max(next_to_receive_index,
                     mapToIndex(sequence_number, receive_base_index) + 1);

        is_received.set(sequence_number);

        auto continuous_received_range =
            getBufferRange() |
            std::ranges::views::take_while([&](const auto &index) {
              return is_received.test(mapToSequenceNumber(index));
            });

        for (const auto &index : continuous_received_range) {
          const SequenceNumberType sequence_number = mapToSequenceNumber(index);
          return_data.append_range(message_views_[sequence_number]);
          is_received.reset(sequence_number);
          ++receive_base_index;
        }

        // ACK
        writeToSocket(*addr, sequence_number, ACK_MESSAGE);
      }
      // lost ack in last window
      else if (isAvailableSequenceNumber(sequence_number +
                                         RECEIVE_WINDOW_SIZE)) {
        // ACK
        writeToSocket(*addr, sequence_number, ACK_MESSAGE);
      }
    }
  }();

  return return_data;
}

int Socket::write(const std::string_view address, const int port,
                  const std::span<const char> data) {

  auto data_pieces = data | std::views::chunk(MAX_MESSAGE_SIZE);
  int total_write_bytes = 0;
  sockaddr_in addr;
  UDP_FillSockAddr(&addr, address.data(), port);

  // handshake
  while (true) {
    writeToSocket(addr, generateControlSegment(ControlSegmentType::HANDSHAKE));
    if (isControlSegment(*readFromSocket<ReadMode::Blocking>(addr)) &&
        getControlSegmentType() == ControlSegmentType::HANDSHAKE) {
      break;
    }
  }

  std::bitset<REAL_DATA_SEQUENCE_NUMBER_RANGE.size()> is_acked;
  auto ack_view =
      REAL_DATA_SEQUENCE_NUMBER_RANGE |
      std::views::transform([&](int i) { return is_acked.test(i); });
  size_t send_base_index = 0;
  size_t next_to_send_index = 0;
  RetransmissionTimer retransmission_timer(500);

  auto getTravellingRange = [&]() {
    return std::views::iota(send_base_index, next_to_send_index);
  };

  auto getAvailableRange = [&]() {
    return std::views::iota(next_to_send_index,
                            std::min(send_base_index + SEND_WINDOW_SIZE,
                                     std::ranges::size(data_pieces)));
  };

  auto drainAllAcks = [&]() -> size_t {
    size_t num_drained_acks = 0;

    while (true) {
      auto result = readFromSocket<ReadMode::NonBlocking>(addr);

      if (!result) {
        // only way to return: no more acks in OS buffer
        if (result.error() == std::errc::resource_unavailable_try_again)
          break;
        else
          throw std::system_error(make_error_code(result.error()));
      }

      auto sequence_number = *result;
      if (interpretAsString(message_views_[sequence_number]) == ACK_MESSAGE &&
          ack_view[sequence_number] == false) {
        is_acked.set(sequence_number);
        ++num_drained_acks;
      }
    }

    auto continuous_acked_range =
        getTravellingRange() | std::views::take_while([&](const auto &index) {
          return ack_view[mapToSequenceNumber(index)] == true;
        });

    size_t continuous_acked_size = 0;

    for (auto index : continuous_acked_range) {
      is_acked.reset(mapToSequenceNumber(index));
      ++continuous_acked_size;
    }

    if (continuous_acked_size > 0) {
      retransmission_timer.restart();
      send_base_index += continuous_acked_size;
    }

    if constexpr (DEBUG_MODE) {
      printLog("drain acks", addr, num_drained_acks, {});

      auto lost_indices =
          getTravellingRange() | std::views::filter([&](auto idx) {
            return !is_acked.test(mapToSequenceNumber(idx));
          });

      std::println("[unacked_list]{}", lost_indices);
    }

    return num_drained_acks;
  };

  // send data
  constexpr size_t DRAIN_ACKS_INTERVAL = 16;
  size_t time_to_drain_acks = DRAIN_ACKS_INTERVAL;

  auto send_data = [&](size_t index) {
    writeToSocket(addr, mapToSequenceNumber(index), data_pieces[index]);
    --time_to_drain_acks;
    if (time_to_drain_acks == 0) [[unlikely]] {
      drainAllAcks();
      time_to_drain_acks = DRAIN_ACKS_INTERVAL;
    }
  };

  auto retransmit_data = [&]() {
    auto lost_data_index =
        getTravellingRange() |
        std::ranges::views::filter([&](const auto &index) {
          return ack_view[mapToSequenceNumber(index)] == false;
        });

    for (const auto &index : lost_data_index) {
      if constexpr (DEBUG_MODE) {
        printLog("retransmit", addr, mapToSequenceNumber(index),
                 interpretAsString(data_pieces[index]));
      }
      send_data(index);
    }
    retransmission_timer.restart();
  };

  while (send_base_index < std::ranges::size(data_pieces)) {
    while (std::ranges::size(getAvailableRange()) > 0) {
      std::ranges::for_each(getAvailableRange(), send_data);
      next_to_send_index += std::ranges::size(getAvailableRange());
    }

    if (drainAllAcks() == 0) {
      if (retransmission_timer.isExpired()) {
        retransmit_data();
      }
      std::this_thread::yield();
    }
  }

  // finish
  while (true) {
    writeToSocket(addr, generateControlSegment(ControlSegmentType::FINISH));

    if (isControlSegment(*readFromSocket<ReadMode::Blocking>(addr)) &&
        getControlSegmentType() == ControlSegmentType::FINISH) {
      break;
    }
  }

  return total_write_bytes;
}
