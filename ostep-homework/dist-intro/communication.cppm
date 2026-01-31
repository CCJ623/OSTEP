module;
#include <array>
#include <chrono>
#include <cstdint>
#include <expected>
#include <limits>
#include <ranges>
#include <span>
#include <string_view>
#include <vector>

#include <netinet/in.h>

export module Communication;

constexpr bool DEBUG_MODE = false;

enum class ControlSegmentType : uint8_t { HANDSHAKE = 0x00, FINISH = 0x01 };
enum class ReadMode { Blocking, NonBlocking };

using SequenceNumberType = uint16_t;
inline constexpr auto SEQUENCE_NUMBER_SIZE{sizeof(SequenceNumberType)};
inline constexpr auto MAX_SEQUENCE_NUMBER =
    std::numeric_limits<SequenceNumberType>::max();
inline constexpr auto MIN_SEQUENCE_NUMBER =
    std::numeric_limits<SequenceNumberType>::min();
inline constexpr std::string_view ACK_MESSAGE{"ACK"};
inline constexpr std::chrono::milliseconds TIME_OUT{3000};
inline constexpr size_t MAXIMUN_TRANSMISSION_UNIT{1472};
inline constexpr size_t MAX_MESSAGE_SIZE{MAXIMUN_TRANSMISSION_UNIT -
                                         SEQUENCE_NUMBER_SIZE};

using MessageType = std::array<uint8_t, MAX_MESSAGE_SIZE>;
inline constexpr size_t BUFFER_SIZE = MAX_SEQUENCE_NUMBER + 1;
inline constexpr auto REAL_DATA_SEQUENCE_NUMBER_RANGE =
    std::views::iota(MIN_SEQUENCE_NUMBER, MAX_SEQUENCE_NUMBER);
// NUM_SEQUENCE_NUMBER_FOR_REAL_DATA = BUFFER_SIZE - CONTROL_BLOCK_SIZE
inline constexpr auto RECEIVE_WINDOW_SIZE =
    REAL_DATA_SEQUENCE_NUMBER_RANGE.size() / 2;
static_assert(RECEIVE_WINDOW_SIZE <=
              REAL_DATA_SEQUENCE_NUMBER_RANGE.size() / 2);
inline constexpr auto SEND_WINDOW_SIZE =
    REAL_DATA_SEQUENCE_NUMBER_RANGE.size() / 2;
static_assert(SEND_WINDOW_SIZE <= REAL_DATA_SEQUENCE_NUMBER_RANGE.size() / 2);

// [[nodiscard]] constexpr auto get_sequence_range() noexcept {
//   return std::views::iota(0, MAX_SEQUENCE_NUMBER);
// }

export class Socket {
public:
  Socket() = delete;
  explicit Socket(int port);

  Socket(const Socket &) = delete;
  Socket &operator=(const Socket &) = delete;

  Socket(Socket &&) noexcept;
  Socket &operator=(Socket &&) noexcept;

  ~Socket();

  std::vector<char> read(sockaddr_in *addr);
  [[nodiscard]] int write(std::string_view address, int port,
                          std::span<const char> data);

private:
  bool is_all_pieces_arrived(SequenceNumberType num_of_pieces) const;
  ControlSegmentType getControlSegmentType() const;

  template <ReadMode Mode = ReadMode::Blocking>
  std::expected<SequenceNumberType, std::errc>
  readFromSocket(sockaddr_in &addr);

  template <std::ranges::contiguous_range R>
  void writeToSocket(sockaddr_in &addr, SequenceNumberType sequence_number,
                     const R &data);
  void writeToSocket(sockaddr_in &addr,
                     const std::span<const std::byte> segment);

  int socket_;
  // std::array<MessageType, BUFFER_SIZE> buffer_;
  std::vector<MessageType> buffer_{BUFFER_SIZE};
  std::array<std::span<uint8_t>, BUFFER_SIZE> message_views_;
};

/*
if sequence number == -1, then end
[sequence number][data]
*/