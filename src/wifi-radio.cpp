#include "AudioFileSourceWebSocket.hpp"
#include "AudioGeneratorWavNonBlock.hpp"
#include "IndexPage.hpp"
#include "Params.hpp"
#include "Router.hpp"
#include "Utils.hpp"
#include "WiFiManager.hpp"
#include <Arduino.h>
#include <AudioOutputI2SNoDAC.h>
#include <DNSServer.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>

// Called when a metadata event occurs (i.e. an ID3 tag, an ICY block, etc.
void MDCallback(void *cbData, const char *type, bool isUnicode,
                const char *string) {
  const char *ptr = reinterpret_cast<const char *>(cbData);
  (void)isUnicode; // Punt this ball for now
  // Note that the type and string may be in PROGMEM, so copy them to RAM for
  // printf
  char s1[32], s2[64];
  strncpy_P(s1, type, sizeof(s1));
  s1[sizeof(s1) - 1] = 0;
  strncpy_P(s2, string, sizeof(s2));
  s2[sizeof(s2) - 1] = 0;
  Serial.printf("METADATA(%s) '%s' = '%s'\n", ptr, s1, s2);
  Serial.flush();
}

// Called when there's a warning or error (like a buffer underflow or decode
// hiccup)
void StatusCallback(void *cbData, int code, const char *string) {
  const char *ptr = reinterpret_cast<const char *>(cbData);
  // Note that the string may be in PROGMEM, so copy it to RAM for printf
  char s1[64];
  strncpy_P(s1, string, sizeof(s1));
  s1[sizeof(s1) - 1] = 0;
  Serial.printf("STATUS(%s) '%d' = '%s'\n", ptr, code, s1);
  Serial.flush();
}

// const char *ssid = "HUAWEI-B593-AC9B"; // your WiFi Name
// const char *password = "5G7LE1EH7F5";  // Your Wifi Password
Params params{
    .newSsid = std::nullopt,
    .newPass = std::nullopt,
    .ssid = "Rabbit",
    .pass = "QwPoAsLk01#73",
    .apssid = "ESP8266 Radio",
    .appass = "QwPoAsLk01#73",
    .ledEnabled = true,
    .pttEnabled = false,
};

DNSServer dnsServer;
AsyncWebServer server(80);
WiFiManager *wifi;
Router *router;
IndexPage *indexPage;

AudioGeneratorWavNonBlock *gen;
AudioOutputI2SNoDAC *out;
AudioFileSourceWebSocket *source;

void setup() {
  Serial.begin(115200);
  LittleFS.begin();
  audioLogger = &Serial;
  out = new AudioOutputI2SNoDAC();
  gen = new AudioGeneratorWavNonBlock();
  source = new AudioFileSourceWebSocket("/ws", gen, out);

  params.setup();
  delay(10);
  Serial.println();
  Serial.println();
  wifi = new WiFiManager(params);
  router = new Router(params, *wifi);
  indexPage = new IndexPage(params, *wifi);

  if (dnsServer.start(53, "esp-radio.lan", params.apIp)) {
    Serial.printf("DNS started: http://esp-radio.lan\n");
  }

  wifi->connect();

  Serial.println("Server started");
  Serial.print("Use this URL to connect: ");
  Serial.printf("http://%s/\n", params.apIp.toString().c_str());
  if (wifi->isConnected()) {
    Serial.println("or:");
    Serial.printf("http://%s/\n", params.localIp.toString().c_str());
  }

  server.addHandler(source->getWs());
  server.serveStatic("/main.js", LittleFS, "/main.js");
  server.addHandler(indexPage);
  server.addHandler(router);

  server.begin();

  source->RegisterMetadataCB(MDCallback, (void *)"source");
  gen->RegisterStatusCB(StatusCallback, (void *)"generator");
}

void loop() {
  dnsServer.processNextRequest();
  if (!source->yielding()) {
    source->loop();
    if (gen->isRunning()) {
      if (!gen->loop()) {
        Serial.println("gen->loop() = false");
        gen->stop();
      }
      params.pttEnabled = true;
    } else {
      params.pttEnabled = false;
    }
  }
  params.loop();
}
