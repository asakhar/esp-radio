#include "AudioFileSourceWebSocket.hpp"
#include <deque>

AudioFileSourceWebSocket::AudioFileSourceWebSocket(const char *url,
                                                   AudioGenerator *gen,
                                                   AudioOutput *out)
    : gen(gen), out(out), ws(url) {
  ws.onEvent([this](AsyncWebSocket *server, AsyncWebSocketClient *client,
                    AwsEventType type, void *arg, uint8_t *data, size_t len) {
    return onEvent(server, client, type, arg, data, len);
  });
}

AudioFileSourceWebSocket::~AudioFileSourceWebSocket() { ws.cleanupClients(); }

uint32_t AudioFileSourceWebSocket::read(void *data, uint32_t len) {
  if (data == NULL) {
    audioLogger->printf_P(
        PSTR("ERROR! AudioFileSourceWebSocket::read passed NULL data\n"));
    return 0;
  }
  return readInternal(data, len, false);
}

uint32_t AudioFileSourceWebSocket::readNonBlock(void *data, uint32_t len) {
  if (data == NULL) {
    audioLogger->printf_P(PSTR(
        "ERROR! AudioFileSourceWebSocket::readNonBlock passed NULL data\n"));
    return 0;
  }
  return readInternal(data, len, true);
}

uint32_t AudioFileSourceWebSocket::readInternal(void *data, uint32_t len,
                                                bool nonBlock) {
  // Serial.printf("readInternal len=%u nonBlock=%d\n", len, nonBlock);
  // if (!isOpen()) {
  //   cb.st(STATUS_DISCONNECTED, PSTR("Stream disconnected"));
  //   size = 0;
  //   pos = 0;
  //   requested = 0;
  //   buff = {};
  //   return 0;
  // }
  if ((size > 0) && (pos >= size))
    return 0;

  // Can't read past EOF...
  if (!nonBlock && !buff.bytesUsed()) {
    int start = millis();
    isYielding = true;
    while ((buff.bytesUsed() < len) && (millis() - start < 50))
      delay(1);
    isYielding = false;
  }

  if (!buff.bytesUsed()) {
    if (!nonBlock) {
      cb.st(STATUS_NODATA, PSTR("No stream data available"));
    }
    return 0;
  }

  noInterrupts();
  int read = buff.memcpyOut(reinterpret_cast<uint8_t *>(data), len);
  pos += read;
  interrupts();

  return read;
}

bool AudioFileSourceWebSocket::seek(int32_t pos, int dir) {
  audioLogger->printf_P(
      PSTR("ERROR! AudioFileSourceWebSocket::seek not implemented!"));
  (void)pos;
  (void)dir;
  return false;
}

bool AudioFileSourceWebSocket::close() {
  Serial.println("Closing all connections!");
  ws.closeAll();
  return true;
}

bool AudioFileSourceWebSocket::isOpen() { return ws.count() == 1; }

uint32_t AudioFileSourceWebSocket::getSize() {
  // Dirty hack
  // return 1240000;
  return size;
}

uint32_t AudioFileSourceWebSocket::getPos() { return pos; }

bool AudioFileSourceWebSocket::loop() {
  if (!isOpen()) {
    return true;
  }
  // auto oldReq = requested;
  noInterrupts();
  long freeSpace = buff.bytesFreeIsr();
  auto to_request = freeSpace - requested;
  if (to_request > (long)(buffSize / 2)) {
    requested += to_request;
  } else {
    to_request = 0;
  }
  interrupts();
  if (to_request) {
    auto requesting = String(to_request);
    // Serial.printf("Requesting='%s', free=%ld, requestedOld=%d,
    // requested=%d\n",
    //               requesting.c_str(), freeSpace, oldReq, requested);
    ws.textAll(requesting);
  }
  return true;
}

void AudioFileSourceWebSocket::handleWebSocketMessage(void *arg, uint8_t *data,
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
    size += read;
    requested -= read;
    interrupts();
    if (read != len) {
      cb.st(STATUS_BUFFER_OVERFLOW, PSTR("Buffer is not big enough"));
    }
    // Serial.printf("Read %zu bytes from WS: '%.4s'\n", read, data);
    if (!gen->isRunning() && pos == 0 && size >= 4096) {
      gen->begin(this, out);
    }
  }
}

void AudioFileSourceWebSocket::onEvent(AsyncWebSocket *server,
                                       AsyncWebSocketClient *client,
                                       AwsEventType type, void *arg,
                                       uint8_t *data, size_t len) {
  switch (type) {
  case WS_EVT_CONNECT:
    Serial.printf("Connected count: %d\n", ws.count());
    if (ws.count() > 1) {
      client->close();
      Serial.println("Only one client supported!");
    } else {
      Serial.printf("WebSocket client #%u connected from %s\n", client->id(),
                    client->remoteIP().toString().c_str());
    }
    break;
  case WS_EVT_DISCONNECT:
    Serial.printf("Connected count: %d\n", ws.count());
    Serial.printf("WebSocket client #%u disconnected\n", client->id());
    if (ws.count() == 0) {
      gen->stop();
    }
    break;
  case WS_EVT_DATA:
    handleWebSocketMessage(arg, data, len);
    break;
  case WS_EVT_PONG:
  case WS_EVT_ERROR:
    break;
  }
}
