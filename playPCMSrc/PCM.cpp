#include "PCM.h"
#include <cstdlib>
#include <sstream>
#include <stdexcept>
#include <iostream>
#include <fstream>
#include <cerrno> 
#include <cstring>

#ifdef _WIN32
#define POPEN wpopen
#define PCLOSE pclose
#else
#define POPEN popen
#define PCLOSE pclose
#endif


bool ffmpegAvailable() {
#ifdef _WIN32
  return std::system("ffmpeg -version >nul 2>nul") == 0;
#else
  return std::system("ffmpeg -version >/dev/null 2>&1") == 0;
#endif
}

PCM::PCM() : filePath(), pipe(nullptr), ended(false), fileSize(0) {}

PCM::~PCM() {
  stop();
}

int PCM::load(const path_t& filePath) {
  stop();
  this->filePath = filePath;

  if (this->filePath.empty()) {
    return -1;
  }

  try {
    fileSize = std::filesystem::file_size(std::filesystem::path(filePath));
  } catch (...) {
    fileSize = 0;
  }

  if (!ffmpegAvailable()) {
    return -2;
  }

  try {
    start();
  } catch (...) {
    return -1;
  }

  return 0;
}

path_t PCM::buildCommand() const {
#ifdef _WIN32
  std::wostringstream cmd;

  cmd << L"ffmpeg -hide_banner -loglevel error "
      << L"-i \"" << filePath << L"\" "
      << L"-f s8 -ac 2 -ar 8000 -acodec pcm_s8 pipe:1";
  return cmd.str();
#else
  std::ostringstream cmd;

  cmd << "ffmpeg -hide_banner -loglevel error "
      << "-i \"" << filePath << "\" "
      << "-f s8 -ac 2 -ar 8000 -acodec pcm_s8 pipe:1";
  return cmd.str();
#endif
}

void PCM::start() {
  stop();

  if (filePath.empty()) {
    throw std::runtime_error("No input file loaded");
  }

#ifdef _WIN32
  std::wstring command = buildCommand();
  pipe = POPEN(command.c_str(), L"rb");
#else
  std::string command = buildCommand();
  pipe = POPEN(command.c_str(), "r");
#endif
if (!pipe) {
    std::cerr << "popen failed, errno=" << errno << " (" << strerror(errno) << ")\n";
    throw std::runtime_error("Could not start ffmpeg");
}
  ended = false;
}

void PCM::stop() {
  if (pipe) {
    PCLOSE(pipe);
    pipe = nullptr;
  }
  ended = true;
}

int PCM::getNextChunk(unsigned char* buffer) {
  return getBytes(buffer, CHUNK_SIZE);
}

int PCM::getBytes(unsigned char* buffer, int size) {
  if (!pipe || size <= 0) {
    return -1;
  }

  std::size_t bytesRead = std::fread(buffer, 1, static_cast<std::size_t>(size), pipe);
  if (bytesRead == 0) {
    if (std::feof(pipe)) {
      ended = true;
      return 0;
    }
    if (std::ferror(pipe)) {
      ended = true;
      return -1;
    }
  }

  return static_cast<int>(bytesRead);
}

bool PCM::eof() {
  return ended;
}

void PCM::reset() {
  start();
}