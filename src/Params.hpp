#pragma once
#include <Arduino.h>
#include <IPAddress.h>
#include <memory>

enum AudioState {
  IDLE,
  RECORDING,
  PLAYING
};

struct Params
{
  static constexpr int ledPin = 2;
  static constexpr int pttPin = 17;
  static constexpr uint16_t remotePort{9000};
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
  IPAddress remoteIp;
  volatile bool dirChange;
  AudioState audioState;

  inline void setup() const
  {
    pinMode(ledPin, OUTPUT);
    pinMode(pttPin, OUTPUT);
    loop();
  }

  inline void loop() const
  {
    digitalWrite(ledPin, ledValue());
    digitalWrite(pttPin, pttValue());
  }

  inline uint8_t ledValue() const { return ledEnabled ? HIGH : LOW; }
  inline uint8_t pttValue() const { return pttEnabled ? LOW : HIGH; }
};
