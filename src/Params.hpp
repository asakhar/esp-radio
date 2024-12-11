#pragma once
#include <Arduino.h>
#include <IPAddress.h>
#include <memory>

struct Params {
  static constexpr int ledPin = 2;
  static constexpr int pttPin = 27;
  String newSsid;
  String newPass;
  String ssid;
  String pass;
  String apssid;
  String appass;
  bool ledEnabled;
  bool pttEnabled;
  IPAddress localIp;
  IPAddress apIp;

  inline void setup() const {
    pinMode(ledPin, OUTPUT);
    pinMode(pttPin, OUTPUT);
    loop();
  }

  inline void loop() const {
    digitalWrite(ledPin, ledValue());
    digitalWrite(pttPin, pttValue());
  }

  inline uint8_t ledValue() const { return ledEnabled ? LOW : HIGH; }
  inline uint8_t pttValue() const { return pttEnabled ? LOW : HIGH; }
};