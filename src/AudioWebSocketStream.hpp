#pragma once
#include "RingBuf.hpp"
#include <AudioTools.h>
// #include <AudioFileSource.h>
// #include <AudioGenerator.h>
#include <ESPAsyncWebServer.h>

class AudioWebSocketStream : public AudioStream {
public:
  AudioWebSocketStream(char const *url);

  ~AudioWebSocketStream() override;

  size_t readBytes(uint8_t *data, size_t len) override;

  int read() override;

  int peek() override;

  inline void flush() override {}

  inline size_t write(uint8_t) override { return not_supported(0); }

  inline size_t write(const uint8_t *, size_t) override {
    return not_supported(0);
  }

  inline bool active() const { return ws.count() != 0; }

  inline operator bool() const { return active(); }

  inline AsyncWebSocket *getWs() { return &ws; }

  inline void loop() { request(); }

private:
  void handleWebSocketMessage(void *arg, uint8_t *data, size_t len);

  void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client,
               AwsEventType type, void *arg, uint8_t *data, size_t len);

  void request();

  AsyncWebSocket ws;
  RingBuf buff{4096 * 4};
  std::atomic<size_t> requested{0};
};
