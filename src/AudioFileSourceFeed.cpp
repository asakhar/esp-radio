#include "AudioFileSourceFeed.hpp"
#include <Arduino.h>

#pragma GCC optimize("O3")

AudioFileSourceFeed::AudioFileSourceFeed(uint32_t buffSizeBytes) {
  buffSize = buffSizeBytes;
  buffer = (uint8_t *)malloc(sizeof(uint8_t) * buffSize);
  if (!buffer)
    audioLogger->printf_P(
        PSTR("Unable to allocate AudioFileSourceFeed::buffer[]\n"));
  else
    Serial.println("Buffer allocated successfully!");
  deallocateBuffer = true;
  writePtr = 0;
  readPtr = 0;
  length = 0;
  totalPos = 0;
  filled = false;
}

AudioFileSourceFeed::AudioFileSourceFeed(void *inBuff, uint32_t buffSizeBytes) {
  buffSize = buffSizeBytes;
  buffer = (uint8_t *)inBuff;
  deallocateBuffer = false;
  writePtr = 0;
  readPtr = 0;
  length = 0;
  totalPos = 0;
  filled = false;
}

AudioFileSourceFeed::~AudioFileSourceFeed() {
  if (deallocateBuffer)
    free(buffer);
  buffer = NULL;
}

bool AudioFileSourceFeed::seek(int32_t pos, int dir) {
  Serial.printf("seek pos=%d, dir=%d\n", pos, dir);
  if (dir == SEEK_CUR && (readPtr + pos) < length) {
    readPtr += pos;
    totalPos += pos;
    return true;
  } else {
    return false;
  }
}

bool AudioFileSourceFeed::close() {
  Serial.printf("close\n");
  if (deallocateBuffer)
    free(buffer);
  buffer = NULL;
  return true;
}

bool AudioFileSourceFeed::isOpen() { return true; }

uint32_t AudioFileSourceFeed::getSize() { return totalPos + getFillLevel(); }

uint32_t AudioFileSourceFeed::getPos() { return readPtr; }

uint32_t AudioFileSourceFeed::getFillLevel() { return length; }

uint32_t AudioFileSourceFeed::read(void *data, uint32_t len) {
  Serial.println("read");
  uint32_t bytes = 0;
  if (!filled) {
    cb.st(STATUS_UNDERFLOW, PSTR("Buffer underflow"));
    return bytes;
  }

  // Pull from buffer until we've got none left or we've satisfied the request
  uint8_t *ptr = reinterpret_cast<uint8_t *>(data);
  uint32_t toReadFromBuffer = (len < length) ? len : length;
  if ((toReadFromBuffer > 0) && (readPtr >= writePtr)) {
    uint32_t toReadToEnd = (toReadFromBuffer < (uint32_t)(buffSize - readPtr))
                               ? toReadFromBuffer
                               : (buffSize - readPtr);
    memcpy(ptr, &buffer[readPtr], toReadToEnd);
    readPtr = (readPtr + toReadToEnd) % buffSize;
    len -= toReadToEnd;
    length -= toReadToEnd;
    ptr += toReadToEnd;
    bytes += toReadToEnd;
    totalPos += toReadToEnd;
    toReadFromBuffer -= toReadToEnd;
  }
  if (toReadFromBuffer > 0) { // We know RP < WP at this point
    memcpy(ptr, &buffer[readPtr], toReadFromBuffer);
    readPtr = (readPtr + toReadFromBuffer) % buffSize;
    len -= toReadFromBuffer;
    length -= toReadFromBuffer;
    ptr += toReadFromBuffer;
    bytes += toReadFromBuffer;
    totalPos += toReadFromBuffer;
    toReadFromBuffer -= toReadFromBuffer;
  }

  if (len) {
    // We're out of buffered data, need to force a complete refill.  Thanks,
    // @armSeb
    readPtr = 0;
    writePtr = 0;
    length = 0;
    filled = false;
    cb.st(STATUS_UNDERFLOW, PSTR("Buffer underflow"));
  }

  return bytes;
}

bool AudioFileSourceFeed::loop() { return true; }
