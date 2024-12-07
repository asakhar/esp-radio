#include <AudioFileSource.h>

class AudioFileSourceFeed : public AudioFileSource {
public:
  AudioFileSourceFeed(uint32_t bufferBytes);
  AudioFileSourceFeed(void *buffer,
                      uint32_t bufferBytes); // Pre-allocated buffer by app
  virtual ~AudioFileSourceFeed() override;

  virtual uint32_t read(void *data, uint32_t len) override;
  virtual bool seek(int32_t pos, int dir) override;
  virtual bool close() override;
  virtual bool isOpen() override;
  virtual uint32_t getSize() override;
  virtual uint32_t getPos() override;
  virtual bool loop() override;

  virtual uint32_t getFillLevel();

  template <typename T> uint32_t push(T &&read, uint32_t len) {
    uint32_t pushed = 0;
    if (readPtr > writePtr && len > 0) {
      if (readPtr == writePtr + 1)
        return pushed;
      Serial.printf("push end len=%u\n", len);
      uint32_t bytesAvailMid = readPtr - writePtr - 1;
      int cnt = min(len, bytesAvailMid);
      cnt = read(&buffer[writePtr], cnt);
      // int cnt = src->readNonBlock(&buffer[writePtr], bytesAvailMid);
      length += cnt;
      len -= cnt;
      pushed += cnt;
      writePtr = (writePtr + cnt) % buffSize;
      return pushed;
    }

    if (buffSize > writePtr && len > 0) {
      Serial.printf("push mid len=%u, buffSize=%u, writePtr=%u\n", len,
                    buffSize, writePtr);
      uint32_t bytesAvailEnd = buffSize - writePtr;
      int cnt = min(len, bytesAvailEnd);
      cnt = read(&buffer[writePtr], cnt);
      // int cnt = src->readNonBlock(&buffer[writePtr], bytesAvailEnd);
      length += cnt;
      len -= cnt;
      pushed += cnt;
      writePtr = (writePtr + cnt) % buffSize;
      Serial.println("push mid finished");
      Serial.flush();
      if (cnt != (int)bytesAvailEnd)
        return pushed;
    }

    if (readPtr > 1 && len > 0) {
      Serial.printf("push start len=%u\n", len);
      uint32_t bytesAvailStart = readPtr - 1;
      int cnt = min(len, bytesAvailStart);
      cnt = read(&buffer[writePtr], cnt);
      // int cnt = src->readNonBlock(&buffer[writePtr], bytesAvailStart);
      length += cnt;
      len -= cnt;
      pushed += cnt;
      writePtr = (writePtr + cnt) % buffSize;
    }
    return pushed;
  }

  enum { STATUS_FILLING = 2, STATUS_UNDERFLOW };

private:
  uint32_t buffSize;
  uint8_t *buffer;
  bool deallocateBuffer;
  uint32_t writePtr;
  uint32_t readPtr;
  uint32_t length;
  bool filled;
  uint32_t totalPos;
};