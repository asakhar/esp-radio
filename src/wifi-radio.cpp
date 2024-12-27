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
    .apssid = "ESP8266 Radio",
    .appass = "QwPoAsLk01#73",
    .ledEnabled = true,
    .pttEnabled = false,
    .dirChange = false,
    .audioRunning = false,
};

DNSServer dnsServer;
AsyncWebServer server(80);
WiFiManager *wifi;
Router *router;
AudioInfo info(20000, 1, 16);
AnalogAudioStream audio;
UDPStream remote;

#if 0
SineWaveGenerator<int16_t> sineWave( 32000);  // subclass of SoundGenerator with max amplitude of 32000
GeneratedSoundStream<int16_t> sound( sineWave);  // Stream generated from sine wave
Throttle throttle(drain);
StreamCopy inCopier(throttle, sound);
#endif

StreamCopy outCopier(audio, remote);
StreamCopy inCopier(remote, audio);

void setup()
{
  Serial.begin(115200);
  // AudioToolsLogger.begin(Serial, AudioToolsLogLevel::Info); // Warning, Info, Error, Debug
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

  AnalogConfig config;
  if (params.pttEnabled)
  {
    remote.begin(9000);
    config = audio.defaultConfig(TX_MODE);
  }
  else
  {
    remote.begin(params.receiverIp, params.receiverPort);
    config = audio.defaultConfig(RX_MODE);
  }
  config.copyFrom(info);
  config.uninstall_driver_on_end = true;
  if (audio.begin(config))
  {
    params.audioRunning = true;
  }
  else
  {
    Serial.println("Failed to start audio capture!");
  }

#if 0
  sineWave.begin(info, N_B4);

  // Define Throttle
  auto cfg = throttle.defaultConfig();
  cfg.copyFrom(info);
  // cfg.correction_us = 100000;
  throttle.begin(cfg);
#endif
}

void loop()
{
  dnsServer.processNextRequest();
  if (params.audioRunning)
  {
    if (params.pttEnabled)
    {
      outCopier.copy();
    }
    else
    {
      inCopier.copy();
    }
  }
  if (params.dirChange)
  {
    AnalogConfig config;
    audio.end();
    remote.end();
    if (params.pttEnabled)
    {
      remote.begin(9000);
      config = audio.defaultConfig(TX_MODE);
    }
    else
    {
      remote.begin(params.receiverIp, params.receiverPort);
      config = audio.defaultConfig(RX_MODE);
    }
    config.copyFrom(info);
    config.uninstall_driver_on_end = true;
    if (audio.begin(config))
    {
      params.audioRunning = true;
      params.dirChange = false;
    }
    else
    {
      Serial.println("Failed to change rx/tx mode!");
      params.audioRunning = false;
    }
    delay(100);
  }
  params.loop();
}
