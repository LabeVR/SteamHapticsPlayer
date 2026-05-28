#include "PCM.h"
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>

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

PCM::~PCM() {}

int PCM::load(const path_t& filePath) {
  this->filePath = filePath;

  if (this->filePath.empty()) {
    return -1;
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
      // maybe wii add volume later
      //<< L"-af acompressor=threshold=-16dB:ratio=3:attack=5:release=80,highshelf=f=400:g=8,volume=2 -f s8 -ac 2 -ar 8000 -acodec pcm_s8 pipe:1";
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


// load file into memory
void PCM::start() {
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
  fileSize = 0;
  while (!ended) {
    uint8_t buff[64];
    if (!pipe) break;
    std::size_t bytesRead = std::fread(buff, 1, 64, pipe);
    
    if (bytesRead == 0) {
      if (std::feof(pipe) || std::ferror(pipe)) {
        ended = true;
        break;
      }
    }
    this->pcmBytes.insert(this->pcmBytes.end(), buff, buff + bytesRead);
  }
  if (pipe) {
    PCLOSE(pipe);
    pipe = nullptr;
  }
  fileSize = pcmBytes.size();
  ended = false;
}

int PCM::getNextChunk(uint8_t* buffer) {
  return getBytes(buffer, CHUNK_SIZE);
}

int PCM::getBytes(uint8_t* buffer, int size) {
  if (size <= 0 || buffer == nullptr) return -1;
  if (readPointer >= pcmBytes.size()) {
    ended = true;
    return 0;
  }

  size_t avail = pcmBytes.size() - readPointer;
  size_t toCopy = static_cast<size_t>(size);
  if (avail < toCopy) toCopy = avail;

  std::memcpy(buffer, pcmBytes.data() + readPointer, toCopy);
  readPointer += toCopy;
  if (readPointer >= pcmBytes.size()) ended = true;
  return static_cast<int>(toCopy);
}


void PCM::reset() {
  this->pcmBytes.clear();
  this->readPointer = 0;
  start();
}