#include <algorithm>
#include <cassert>
#include <cstdint>
#include <fstream>
#include <ranges>
#include <vector>

#include "check_functions.h"

using namespace std;

constexpr size_t BLOCK_SIZE = 4 * 1024; // 4KB

int main(int argc, char *argv[]) {
  assert(argc == 3);

  ifstream input_file_stream{argv[1], ios::binary | ios::ate};
  assert(input_file_stream.is_open());

  vector<byte> data;
  auto file_size = input_file_stream.tellg();
  data.resize(file_size);

  input_file_stream.seekg(0, ios::beg);
  input_file_stream.read(reinterpret_cast<char *>(data.data()), file_size);

  auto num_blocks =
      (static_cast<size_t>(file_size) + BLOCK_SIZE - 1) / BLOCK_SIZE;
  vector<byte> checksums;
  checksums.reserve(num_blocks);

  ranges::for_each(views::chunk(data, num_blocks),
                   [&checksums](auto &&block_view) {
                     checksums.push_back(byte{crc_checksum<uint8_t, 8>(
                         block_view, 0x07, 0x00, 0x00, false, false)});
                   });

  ofstream output_file_stream{argv[2], ios::binary | ios::trunc};
  assert(output_file_stream.is_open());
  output_file_stream.write(reinterpret_cast<const char *>(checksums.data()),
                           checksums.size());
}