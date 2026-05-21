#include "PCM.h"
#include <stdexcept>

PCM::PCM(const std::string& filePath) {
  file.open(filePath, std::ios::binary);
  if (!file.is_open()) {
    throw std::runtime_error("Could not open PCM file: " + filePath);
  }
}

PCM::~PCM() {
  if (file.is_open()) {
    file.close();
  }
}

int PCM::getNextChunk(unsigned char* buffer) {
  if (!file.is_open()) {
    return -1;
  }
  file.read(reinterpret_cast<char*>(buffer), CHUNK_SIZE);
  std::streamsize bytesRead = file.gcount();

  if (file.fail() && !file.eof()) {
    return -1;  // error
  }

  return static_cast<int>(bytesRead);
}

int PCM::getBytes(unsigned char* buffer, int size) {
  if (!file.is_open()) return -1;
  if (size <= 0) return 0;

  file.read(reinterpret_cast<char*>(buffer), size);
  std::streamsize bytesRead = file.gcount();

  if (file.fail() && !file.eof()) return -1;
  return static_cast<int>(bytesRead);
}

bool PCM::eof()  {
  return file.eof();
}

void PCM::reset() {
  if (file.is_open()) {
    file.clear();
    file.seekg(0, std::ios::beg);
  }
}