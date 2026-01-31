
#include <chrono>
#include <filesystem>
#include <fstream>
#include <netinet/in.h>
#include <print>
#include <string_view>
#include <thread>
#include <vector>

import Communication;

using namespace std;
namespace fs = filesystem;

constexpr int SOURCE_PORT = 6666;
constexpr int DESTINATION_PORT = 8888;
constexpr string_view RECEIVED_FILE_PATH = "received.txt";

int main(int argc, char *argv[]) {
  if (argc != 2) {
    throw runtime_error{"num of args must be 2"};
  }

  Socket client{SOURCE_PORT};
  Socket server{DESTINATION_PORT};

  fs::path file_path = argv[1];
  if (!fs::exists(file_path)) {
    throw runtime_error{"file doesn't exist"};
  }

  vector<char> source_data;
  vector<char> received_data;
  auto file_size = fs::file_size(file_path);
  ifstream input{file_path, ios::binary};
  source_data.resize(file_size);
  input.read(source_data.data(), static_cast<streamsize>(file_size));

  auto start = chrono::high_resolution_clock::now();
  {
    jthread server_thread{[&received_data, &server]() {
      sockaddr_in addr;
      received_data = server.read(&addr);
    }};
    jthread client_thread{[&client, &source_data]() {
      auto num_write_bytes = client.write("127.0.0.1", DESTINATION_PORT,
                                          source_data) == source_data.size();
    }};
  }
  auto end = chrono::high_resolution_clock::now();
  auto duration = end - start;

  ofstream output{fs::path{RECEIVED_FILE_PATH}, ios::binary};
  output.write(received_data.data(), received_data.size());

  if (received_data == source_data) {
    println("successful");
  } else {
    println("failed");
  }

  std::println("\n{:=^50}", " Bandwidth Report ");
  std::println("Total Data:    {}MB",
               static_cast<double>(file_size) / 1024 / 1024);
  std::println("Time Cost:     {}", duration);
  std::println("Throughput:    {:.2f} MB/s", static_cast<double>(file_size) /
                                                 1024 / 1024 /
                                                 duration.count() * 1e9);
  std::println("{:=^50}\n", "");
}