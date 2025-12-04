#include <cassert>
#include <chrono>
#include <cstdint>
#include <fstream>
#include <print>

#include "check_functions.h"

using namespace std;

void benchmark(size_t length) {}

int main(int argc, char *argv[]) {
  assert(argc == 2);

  ifstream input_file_stream{argv[1], ios::binary | ios::ate};
  assert(input_file_stream);

  auto file_size = input_file_stream.tellg();
  vector<byte> data;
  data.resize(file_size);

  input_file_stream.seekg(0, ios::beg);
  input_file_stream.read(reinterpret_cast<char *>(data.data()), file_size);

  decltype(chrono::high_resolution_clock::now()) start, end;

  start = chrono::high_resolution_clock::now();
  auto xor_result = xor_checksum(data);
  end = chrono::high_resolution_clock::now();
  println("xor checksum: {:02X}\tcost: {}", xor_result, end - start);

  start = chrono::high_resolution_clock::now();
  auto fletcher_result = fletcher_checksum(data);
  end = chrono::high_resolution_clock::now();
  println("fletcher checksum: {:02X},{:02X}\tcost: {}", fletcher_result.first,
          fletcher_result.second, end - start);

  start = chrono::high_resolution_clock::now();
  // crc-16/USB
  auto crc_result =
      crc_checksum<uint16_t, 16>(data, 0x8005, 0xFFFF, 0xFFFF, true, true);
  end = chrono::high_resolution_clock::now();
  println("crc-16/USB checksum: {:02X}\tcost: {}", crc_result, end - start);
}