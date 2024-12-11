#pragma once
#include <Arduino.h>
#ifdef ESP32
#include <WiFi.h>
#else
#include <ESP8266WiFi.h>
#endif
#include <Params.hpp>

class WiFiManager {
public:
  explicit WiFiManager(Params &params) : params(params) {
    WiFi.hostname("esp-radio");
    WiFi.mode(WIFI_AP_STA);
    if (!WiFi.softAP(params.apssid, params.appass)) {
      Serial.println("ERROR: Soft AP creation failed.");
      while (1)
        ;
    }
    params.apIp = WiFi.softAPIP(); // IP address for AP mode
  }
  inline bool connect() {
    auto newssid = params.newSsid;
    if (newssid.isEmpty())
      newssid = params.ssid;
    auto newpass = params.newPass;
    if (newpass.isEmpty())
      newpass = params.pass;
    int attempts = 0;
    Serial.print("Connecting to ");
    Serial.println(newssid);
    WiFi.begin(newssid, newpass);
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
      if (attempts++ > 20) {
        return false;
      }
    }
    if (params.newSsid) {
      params.ssid = newssid;
      params.newSsid.clear();
    }
    if (params.newPass) {
      params.pass = newpass;
      params.newPass.clear();
    }
    Serial.println("");
    Serial.print("WiFi connected to ");
    Serial.print(newssid);
    Serial.println("!");
    params.localIp = WiFi.localIP();
    Serial.print("Use this URL to connect: ");
    Serial.print("http://");
    Serial.print(params.localIp.toString().c_str());
    Serial.println("/");
    return isConnected();
  }
  inline bool isConnected() const { return WiFi.isConnected(); }

private:
  Params &params;
};