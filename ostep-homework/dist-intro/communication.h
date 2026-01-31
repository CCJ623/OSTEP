#include "udp.h"
#include <array>
#include <chrono>
#include <cstdint>
#include <limits>
#include <span>
#include <stdexcept>
#include <string_view>
#include <vector>

constexpr bool DEBUG_MODE = true;

class Socket {
public:
  using SequenceNumberType = uint8_t;
  constexpr static auto SEQUENCE_NUMBER_SIZE{sizeof(SequenceNumberType)};
  constexpr static auto MAX_SEQUENCE_NUMBER =
      std::numeric_limits<SequenceNumberType>::max();
  constexpr static auto MIN_SEQUENCE_NUMBER =
      std::numeric_limits<SequenceNumberType>::min();

  Socket() = delete;
  Socket(int port) : socket_(UDP_Open(port)) {
    if (socket_ == -1) {
      throw std::runtime_error{"open socket error"};
    }
  }

  ~Socket() { UDP_Close(socket_); }

  std::vector<char> read(struct sockaddr_in *addr);
  [[nodiscard]] int write(std::string_view address, int port,
                          std::span<const char> data);

private:
  constexpr static std::string_view ACK_MESSAGE{"ACK"};
  constexpr static std::chrono::milliseconds TIME_OUT{3000};
  constexpr static size_t MAXIMUN_TRANSMISSION_UNIT{1472};
  constexpr static size_t MAX_MESSAGE_SIZE{MAXIMUN_TRANSMISSION_UNIT -
                                           SEQUENCE_NUMBER_SIZE};
  constexpr static size_t BUFFER_SIZE = MAX_SEQUENCE_NUMBER + 1;

  using MessageType = std::array<uint8_t, MAX_MESSAGE_SIZE>;

  bool is_all_pieces_arrived(SequenceNumberType num_of_pieces) const;
  SequenceNumberType readFromSocket(struct sockaddr_in &addr);

  template <std::ranges::contiguous_range R>
  void writeToSocket(struct sockaddr_in &addr,
                     SequenceNumberType sequence_number, const R &data);

  int socket_;
  std::array<MessageType, BUFFER_SIZE> buffer_;
  std::array<std::span<uint8_t>, BUFFER_SIZE> message_views_;
  std::array<bool, BUFFER_SIZE> is_updated_;
};

/*
if sequence number == -1, then end
[sequence number][data]
*/