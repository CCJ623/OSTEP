#include <cstdint>
#include <fstream>
#include <print>

#include "check_functions.h"

using namespace std;

int main(int argc, char *argv[]) {
  // vector<uint8_t> input = {0x01, 0x02};
  ifstream input(argv[1]);

  auto result =
      crc_checksum<uint32_t, 16>(input, 0x8005, 0xFFFF, 0xFFFF, true, true);
  println("crc-16 checksum: {:02X}", result);
}