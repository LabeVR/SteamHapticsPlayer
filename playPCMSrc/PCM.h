#pragma once

#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <string>

class PCM {
private:
  std::wstring filePath;
  FILE* pipe = nullptr;
  bool ended = false;

  static constexpr int CHUNK_SIZE = 8;
  
  std::wstring buildCommand() const;
  void start();
  
  public:
  std::uintmax_t fileSize;
  PCM();
  ~PCM();
  
  void stop();
  // Returns 0 on success, -1 on failure.
  int load(const std::wstring& filePath);

  int getNextChunk(unsigned char* buffer);
  // Read up to `size` bytes into buffer. Returns number of bytes read, 0 on EOF, or -1 on error.
  int getBytes(unsigned char* buffer, int size);

  bool eof();
  void reset();
};