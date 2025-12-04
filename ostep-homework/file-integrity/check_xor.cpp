#include <assert.h>
#include <fstream>
#include <print>

#include "check_functions.h"

using namespace std;

int main(int argc, char *argv[]) {
  assert(argc == 2);
  ifstream input_file_stream(argv[1]);

  println("xor checksum: {:02X}", xor_checksum(input_file_stream));
}