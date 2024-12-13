#include "AudioWebSocketStream.hpp"
#include "Params.hpp"
#include "Router.hpp"
#include "WiFiManager.hpp"
#include <Arduino.h>
#include <DNSServer.h>
#ifdef ESP32
#include <AsyncTCP.h>
#include <WiFi.h>
#else
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#endif
#include <AudioTools.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>

// const char *ssid = "HUAWEI-B593-AC9B"; // your WiFi Name
// const char *password = "5G7LE1EH7F5";  // Your Wifi Password
Params params{
    .newSsid = "",
    .newPass = "",
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
AudioInfo info(44100, 1, 16);
AnalogAudioStream out;
EncodedAudioStream dec(&out, new WAVDecoder());
AudioWebSocketStream source("/ws");

StreamCopy copier(dec, source);

void setup() {
  Serial.begin(115200);
  LittleFS.begin();

  params.setup();
  delay(10);
  Serial.println();
  Serial.println();
  wifi = new WiFiManager(params);
  router = new Router(params, *wifi);

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

  server.addHandler(source.getWs());
  server.serveStatic("/main.js", LittleFS, "/main.js");
  server.serveStatic("/index.html", LittleFS, "/index.html");
  server.on("/", [](AsyncWebServerRequest *request) {
    request->redirect("/index.html");
    return;
  });
  server.addHandler(router);

  server.begin();

  auto config = out.defaultConfig(TX_MODE);
  config.copyFrom(info); 
  out.begin(config);
  dec.begin();
}

void loop() {
  dnsServer.processNextRequest();
  source.loop();
  copier.copy();
  params.loop();
}
