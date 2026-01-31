#include <arpa/inet.h>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <ios>
#include <netinet/in.h>
#include <span>
#include <stdexcept>
#include <string_view>
#include <vector>

import Communication;

using namespace std;
namespace fs = filesystem;

constexpr int PORT = 8888;
constexpr size_t BUFFER_SIZE = 1024;
constexpr string_view SEND_MESSAGE = "Hello World!";
constexpr string_view DESTINATION_ADDRESS = "127.0.0.1";
constexpr int DESTINATION_PORT = 6666;

int main(int argc, char *argv[]) {
  auto socket = Socket(PORT);
  sockaddr_in address;
  span<const char> data_view;
  vector<char> data;

  if (argc == 1) {
    data_view = SEND_MESSAGE;
  } else if (argc == 2) {
    fs::path file_path = argv[1];
    if (!fs::exists(file_path)) {
      throw runtime_error{"can't open file"};
    }

    auto file_size = fs::file_size(file_path);
    ifstream input{file_path, ios::binary};
    data.resize(file_size);
    data_view = data;
  }

  auto num_write_bytes =
      socket.write(DESTINATION_ADDRESS, DESTINATION_PORT, data_view);
}