#pragma once
#include <AudioFileSource.h>
#include <AudioGenerator.h>
#include <ESPAsyncWebServer.h>
#include "RingBuf.hpp"

class AudioFileSourceWebSocket : public AudioFileSource {
public:
  AudioFileSourceWebSocket(char const *url, AudioGenerator *gen,
                           AudioOutput *out);
  virtual ~AudioFileSourceWebSocket() override;

  virtual uint32_t read(void *data, uint32_t len) override;
  virtual uint32_t readNonBlock(void *data, uint32_t len) override;
  virtual bool seek(int32_t pos, int dir) override;
  virtual bool close() override;
  virtual bool isOpen() override;
  virtual uint32_t getSize() override;
  virtual uint32_t getPos() override;
  bool loop() override;
  inline AsyncWebSocket *getWs() { return &ws; }
  inline bool yielding() const { return isYielding; }

  enum {
    STATUS_HTTPFAIL = 2,
    STATUS_DISCONNECTED,
    STATUS_RECONNECTING,
    STATUS_RECONNECTED,
    STATUS_NODATA,
    STATUS_BUFFER_OVERFLOW
  };

private:
  virtual uint32_t readInternal(void *data, uint32_t len, bool nonBlock);
  void handleWebSocketMessage(void *arg, uint8_t *data, size_t len);

  void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client,
               AwsEventType type, void *arg, uint8_t *data, size_t len);
  AudioGenerator *gen;
  AudioOutput *out;
  AsyncWebSocket ws;
  RingBuf<uint8_t, 4096*4> buff;
  bool isYielding{false};
  int pos{0};
  int size{0};
  int requested{0};
};
