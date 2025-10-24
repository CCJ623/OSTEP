#include <assert.h>
#include <fcntl.h>
#include <unistd.h>

#include <iostream>
#include <iterator>
#include <print>
#include <ranges>
#include <string>

#include "include/cxxopts.hpp"

using namespace std;

int main(int argc, char* argv[]) {
  cxxopts::Options option("tail", "print out tail lines of file");
  option.add_options()("n,number", "number of tail lines",
                       cxxopts::value<size_t>()->default_value("1"))(
      "h,help", "print help")("p,path", "file path", cxxopts::value<string>());
  auto arguments = option.parse(argc, argv);

  if (arguments.contains("help")) {
    print("{}", option.help());
    return 0;
  }

  if (!arguments.contains("path")) {
    print("must input path\n");
    return -1;
  }

  string path = arguments["path"].as<string>();
  size_t n = arguments["number"].as<size_t>();

  int fd = open(path.c_str(), O_RDONLY);
  assert(fd != -1);

  string buffer;
  size_t file_size = lseek(fd, 0, SEEK_END);
  size_t read_size = 0;
  while (read_size < file_size && n != 0) {
    lseek(fd, -(read_size + 1), SEEK_END);
    char ch;
    read_size += read(fd, &ch, 1);
    if (ch == '\n') {
      --n;
    }
    buffer.push_back(ch);
  }
  if (buffer.back() == '\n') {
    buffer.pop_back();
  }
  ranges::copy(ranges::views::reverse(buffer), ostream_iterator<char>(cout));
  print("\n");
}