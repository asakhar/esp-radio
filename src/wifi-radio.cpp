#include "IndexPage.hpp"
#include "Params.hpp"
#include "Router.hpp"
#include "Utils.hpp"
#include "WiFiManager.hpp"
#include <Arduino.h>
#include <AudioFileSourceBuffer.h>
#include <AudioFileSourceFeed.hpp>
#include <AudioFileSourceHTTPStream.h>
#include <AudioGeneratorMP3a.h>
#include <AudioOutputI2SNoDAC.h>
#include <DNSServer.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>

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

constexpr int bufSize = 6144;

DNSServer dnsServer;
AsyncWebServer server(80);
WiFiManager *wifi;
Router *router;
IndexPage *indexPage;

AudioGeneratorMP3a *mp3;
AudioOutputI2SNoDAC *out;
AudioFileSourceFeed *source;

void setup() {
  Serial.begin(115200);
  LittleFS.begin();
  audioLogger = &Serial;
  source = new AudioFileSourceFeed(bufSize);
  out = new AudioOutputI2SNoDAC();
  mp3 = new AudioGeneratorMP3a();

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

  server.serveStatic("/index.js", LittleFS, "/index.js");
  server.addHandler(indexPage);
  server.addHandler(router);

  server.begin();
}

void loop() {
  dnsServer.processNextRequest();
  if (mp3->isRunning()) {
    if (!mp3->loop())
      mp3->stop();
  }
  params.loop();
}
