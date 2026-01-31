#include <arpa/inet.h>
#include <array>
#include <cassert>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <netinet/in.h>
#include <string_view>

import Communication;

using namespace std;
namespace fs = filesystem;

constexpr int PORT = 6666;
constexpr size_t BUFFER_SIZE = 1024;
constexpr string_view RECEIVED_FILE_PATH = "received.txt";

int main() {
  auto socket = Socket(PORT);
  array<char, BUFFER_SIZE> buffer;

  while (true) {
    sockaddr_in address;
    auto message = socket.read(&address);

    string_view client_address = inet_ntoa(address.sin_addr);
    int client_port = ntohs(address.sin_port);

    ofstream output{fs::path{RECEIVED_FILE_PATH}, ios::binary};
    output.write(message.data(), message.size());
  }
}