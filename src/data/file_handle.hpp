#pragma once
#include <fstream>
#include <string>

namespace insura::data {
class FileHandler {
 public:
  explicit FileHandler(std::string filepath, std::ios::openmode mode);
  ~FileHandler();
  std::fstream& getStream();

 private:
  std::fstream file_;
};

}  // namespace insura::data
