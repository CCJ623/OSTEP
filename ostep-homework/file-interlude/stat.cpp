#include <assert.h>
#include <iostream>
#include <print>
#include <string_view>
#include <sys/stat.h>

using namespace std;

int main(int argc, char *argv[]) {
  assert(argc == 2);

  string_view FILE_PATH = argv[1];
  struct stat s;
  if (stat(argv[1], &s) != 0) {
    cerr << "Can't open path: \n" << FILE_PATH;
    return -1;
  }

  print(R"(
name: {}
size: {}
number of blocks allocated: {}
reference(link) count: {}
)",
        FILE_PATH, s.st_size, s.st_blocks, s.st_nlink);
}