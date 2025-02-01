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
#include <AudioTools/Communication/UDPStream.h>

// const char *ssid = "HUAWEI-B593-AC9B"; // your WiFi Name
// const char *password = "5G7LE1EH7F5";  // Your Wifi Password
Params params{
    .newSsid = "",
    .newPass = "",
    .ssid = "Rabbit",
    .pass = "QwPoAsLk01#73",
    .apssid = "Esp Radio",
    .appass = "QwPoAsLk01#73",
    .ledEnabled = true,
    .pttEnabled = false,
    .dirChange = false,
    .audioState = IDLE,
};

DNSServer dnsServer;
AsyncWebServer server(80);
WiFiManager *wifi;
Router *router;
AudioInfo dacInfo(24000, 1, 16);
AudioInfo adcInfo(24000, 1, 32);
AudioInfo remoteInfo(8000, 1, 16);

I2SStream audioPlay;
I2SStream audioRecord;

UDPStream remotePlay;
UDPStream remoteRecord;

ResampleStream resamplePlay(audioPlay);
FormatConverterStream resampleRecord(remoteRecord);
// ResampleStream resampleRecord(remoteRecord);

StreamCopy playCopier(resamplePlay, remotePlay);
StreamCopy recordCopier(resampleRecord, audioRecord);

void setup()
{
  // Serial.begin(115200);
  // AudioToolsLogger.begin(Serial, AudioToolsLogLevel::Debug); // Warning, Info, Error, Debug
  LittleFS.begin();

  params.setup();
  delay(10);
  Serial.println();
  Serial.println();
  wifi = new WiFiManager(params);
  router = new Router(params, *wifi);

  if (dnsServer.start(53, "esp-radio.lan", params.apIp))
  {
    Serial.printf("DNS started: http://esp-radio.lan\n");
  }

  wifi->connect();

  Serial.println("Server started");
  Serial.print("Use this URL to connect: ");
  Serial.printf("http://%s/\n", params.apIp.toString().c_str());
  if (wifi->isConnected())
  {
    Serial.println("or:");
    Serial.printf("http://%s/\n", params.localIp.toString().c_str());
  }

  server.serveStatic("/main.js", LittleFS, "/main.js");
  server.serveStatic("/index.html", LittleFS, "/index.html");
  server.on("/", [](AsyncWebServerRequest *request)
            {
    request->redirect("/index.html");
    return; });
  server.addHandler(router);

  server.begin();
}

void processAudioChanges()
{
  I2SConfig config;
  switch (params.audioState)
  {
  case PLAYING:
    audioPlay.end();
    remotePlay.end();
    resamplePlay.end();
    break;
  case RECORDING:
    audioRecord.end();
    remoteRecord.end();
    resampleRecord.end();
    break;
  default:
    break;
  }
  params.audioState = IDLE;
  if (params.pttEnabled)
  {
    remotePlay.begin(9000);
    config = audioPlay.defaultConfig(TX_MODE);
    config.copyFrom(dacInfo);
    config.pin_data = GPIO_NUM_25;
    config.pin_bck = GPIO_NUM_26;
    config.pin_ws = GPIO_NUM_27;

    if (audioPlay.begin(config))
    {
      params.audioState = PLAYING;
      resamplePlay.begin(remoteInfo, dacInfo);
    }
  }
  else
  {
    if (!params.remoteIp)
    {
      return;
    }
    remoteRecord.begin(params.remoteIp, params.remotePort);
    config = audioRecord.defaultConfig(RX_MODE);
    config.copyFrom(adcInfo);
    config.pin_data_rx = GPIO_NUM_14;
    config.pin_data = GPIO_NUM_14;
    config.pin_bck = GPIO_NUM_12;
    config.pin_ws = GPIO_NUM_13;
    config.pin_mck = GPIO_NUM_1;
    config.use_apll = true;
    config.fixed_mclk = 256 * config.sample_rate;

    if (audioRecord.begin(config))
    {
      Serial.printf("Started sending to %s:%u\n", params.remoteIp.toString().c_str(), params.remotePort);
      params.audioState = RECORDING;
      resampleRecord.begin(adcInfo, remoteInfo);
    }
  }
  if (params.audioState == IDLE)
  {
    Serial.println("Failed to change rx/tx mode!");
  }
  delay(100);
}

void loop()
{
  dnsServer.processNextRequest();
  switch (params.audioState)
  {
  case PLAYING:
    playCopier.copy();
    break;
  case RECORDING:
    recordCopier.copy();
    break;
  default:
    break;
  }
  if (params.dirChange)
  {
    Serial.printf("Direction change: %s\n", params.pttEnabled ? "out" : "in");
    processAudioChanges();
    params.dirChange = false;
  }
  params.loop();
}
