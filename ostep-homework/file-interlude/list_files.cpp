#include <dirent.h>

#include <cassert>
#include <print>
#include <string>
#include <string_view>

#include "include/cxxopts.hpp"
#include "sys/stat.h"

using namespace std;

int main(int argc, char* argv[]) {
  cxxopts::Options options("list_files", "list all files in a given directory");
  options.add_options()("l,list_all", "print out information about each file")(
      "p,path", "directory path", cxxopts::value<string>()->default_value("."))(
      "h,help", "print help");

  auto arguments = options.parse(argc, argv);
  if (arguments.contains("help")) {
    print("{}", options.help());
    return 0;
  }

  string path = ".";
  if (arguments.contains("path")) {
    path = arguments["path"].as<string>();
  }

  auto directory = opendir(path.data());
  assert(directory != nullptr);

  decltype(readdir(directory)) directory_entry;

  while ((directory_entry = readdir(directory)) != nullptr) {
    print("---------------------------------\n");

    print("name: {}\ntype: {}\n", directory_entry->d_name,
          directory_entry->d_type);
    if (arguments["list_all"].as<bool>()) {
      string entry_path = path + '/' + directory_entry->d_name;
      struct stat s;
      stat(entry_path.c_str(), &s);

      print(R"(size: {}
owner: {}
group: {}
permission: {}
)",
            s.st_size, s.st_uid, s.st_gid, s.st_mode);
    }

    print("---------------------------------\n");
  }
}