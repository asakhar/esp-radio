#include "AudioWebSocketStream.hpp"
#include <limits>

AudioWebSocketStream::AudioWebSocketStream(const char *url) : ws(url) {
  ws.onEvent([this](AsyncWebSocket *server, AsyncWebSocketClient *client,
                    AwsEventType type, void *arg, uint8_t *data, size_t len) {
    return onEvent(server, client, type, arg, data, len);
  });
}

AudioWebSocketStream::~AudioWebSocketStream() { ws.cleanupClients(); }

size_t AudioWebSocketStream::readBytes(uint8_t *data, size_t len) {
  request();
  auto read = buff.memcpyOut(data, len);
  return read == 0 ? -1 : read;
}

int AudioWebSocketStream::read() {
  request();
  uint8_t val;
  auto read = buff.memcpyOut(&val, 1);
  return read == 0 ? -1 : val;
}

int AudioWebSocketStream::peek() {
  request();
  return peek();
}

void AudioWebSocketStream::request() {
  if (!active()) {
    return;
  }
  size_t to_request = 0;
  do {
    auto vacant = buff.vacant();
    auto req = requested.load();
    to_request = vacant - req;
    if (to_request < 1024) {
      return;
    }
    if (requested.compare_exchange_weak(req, req + to_request)) {
      break;
    }
  } while (true);
  if (to_request) {
    char buffer[std::numeric_limits<size_t>::digits10 + 1];
    snprintf(buffer, sizeof(buffer), "%lu", to_request);
    ws.textAll(buffer);
  }
}

void AudioWebSocketStream::handleWebSocketMessage(void *arg, uint8_t *data,
                                                  size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo *)arg;
  // Serial.printf(
  //     "Message: final=%d index=%llu info.len=%llu len=%zu opcode=%d\n",
  //     info->final, info->index, info->len, len, info->opcode);
  // if (info->final && info->index == 0 && info->len == len &&
  // info->opcode == WS_BINARY) {
  if (info->final && info->opcode == WS_BINARY) {
    noInterrupts();
    auto read = buff.memcpyIn(data, len);
    requested -= read;
    interrupts();
    if (read != len) {
      LOGE("Buffer is not big enough");
    }
    // Serial.printf("Read %zu bytes from WS: '%.4s'\n", read, data);
  }
}

void AudioWebSocketStream::onEvent(AsyncWebSocket *server,
                                   AsyncWebSocketClient *client,
                                   AwsEventType type, void *arg, uint8_t *data,
                                   size_t len) {
  switch (type) {
  case WS_EVT_CONNECT:
    if (ws.count() > 1) {
      client->close();
      LOGE("Only one client supported!");
    } else {
      LOGI("WebSocket client #%u connected from %s", client->id(),
           client->remoteIP().toString().c_str());
      if (!buff.clear()) {
        LOGE("Failed to clear buffer!");
      }
    }
    break;
  case WS_EVT_DISCONNECT:
    LOGI("WebSocket client #%u disconnected", client->id());
    break;
  case WS_EVT_DATA:
    handleWebSocketMessage(arg, data, len);
    break;
  case WS_EVT_PONG:
  case WS_EVT_ERROR:
    break;
  }
}
