#pragma once

#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <string>
#include <vector>

#ifdef _WIN32
using path_t = std::wstring;
#else
using path_t = std::string;
#endif

class PCM {
private:
  path_t filePath;
  FILE* pipe = nullptr;
  bool ended = false;
  std::vector<uint8_t> pcmBytes = std::vector<uint8_t>();
  size_t readPointer = 0;

  static constexpr int CHUNK_SIZE = 8;
  
  path_t buildCommand() const;
  void start();
  
  public:
  std::uintmax_t fileSize;
  PCM();
  ~PCM();
  
  int load(const path_t& filePath);

  int getNextChunk(uint8_t* buffer);
  // Read up to `size` bytes into buffer. Returns number of bytes read, 0 on EOF, -1 on error
  int getBytes(uint8_t* buffer, int size);

  void reset();
};