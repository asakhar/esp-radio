#pragma once

#include "AudioGenerator.h"
#include "RingBuf.hpp"

class AudioGeneratorWavNonBlock : public AudioGenerator {
public:
  AudioGeneratorWavNonBlock();
  virtual ~AudioGeneratorWavNonBlock() = default;
  virtual bool begin(AudioFileSource *source, AudioOutput *output) override;
  virtual bool loop() override;
  virtual bool stop() override;
  virtual bool isRunning() override;

private:
  bool ReadU32(uint32_t *dest) {
    return file->read(reinterpret_cast<uint8_t *>(dest), 4);
  }
  bool ReadU16(uint16_t *dest) {
    return file->read(reinterpret_cast<uint8_t *>(dest), 2);
  }
  bool ReadU8(uint8_t *dest) {
    return file->read(reinterpret_cast<uint8_t *>(dest), 1);
  }
  bool GetBufferedData(size_t bytes, void *dest);
  bool ReadWAVInfo();

protected:
  // WAV info
  uint16_t channels;
  uint32_t sampleRate;
  uint16_t bitsPerSample;

  uint32_t availBytes;

  // We need to buffer some data in-RAM to avoid doing 1000s of small reads
  RingBuf<uint8_t, 128> buffer;
  uint8_t scratch[128];
};
