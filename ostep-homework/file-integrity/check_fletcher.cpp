#include <assert.h>
#include <fstream>
#include <print>

#include "check_functions.h"

using namespace std;

int main(int argc, char *argv[]) {
  assert(argc == 2);
  ifstream input_file_stream(argv[1]);

  auto result = fletcher_checksum(input_file_stream);
  println("fletcher checksum: {:02X},{:02X}", result.first, result.second);
}