#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

#include <algorithm>
#include <array>
#include <initializer_list>
#include <print>
#include <string>
#include <string_view>

#include "include/cxxopts.hpp"

using namespace std;

void printDirectory(string& path) {
  print("{}\n", path);
  auto directory = opendir(path.data());
  decltype(readdir(directory)) directory_entry;
  while ((directory_entry = readdir(directory)) != nullptr) {
    if (directory_entry->d_type == DT_DIR) {
      if (ranges::contains(initializer_list<string_view>{".", ".."},
                           directory_entry->d_name)) {
        continue;
      }

      path.append("/").append(directory_entry->d_name);
      printDirectory(path);
      path.resize(path.length() - strlen(directory_entry->d_name) - 1);
    } else {
      print("{}/{}\n", path, directory_entry->d_name);
    }
  }
  closedir(directory);
}

int main(int argc, char* argv[]) {
  cxxopts::Options option("recursive search", "find a file/directory");
  option.add_options()("h,help", "print help")("name", "name of file/directory",
                                               cxxopts::value<string>());
  option.parse_positional("name");
  auto arguments = option.parse(argc, argv);

  if (arguments.contains("help")) {
    print("{}", option.help());
    return 0;
  }

  string name;
  if (arguments.contains("name")) {
    name = arguments["name"].as<string>();
  } else {
    array<char, 256> buffer;
    getcwd(buffer.data(), buffer.size());
    name = buffer.data();
  }

  struct stat s;
  stat(name.c_str(), &s);

  if (S_ISDIR(s.st_mode)) {
    printDirectory(name);
  } else {
    print("{}\n", name);
  }
}