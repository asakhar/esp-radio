#pragma once
#include "Params.hpp"
#include "WiFiManager.hpp"
#include <ESPAsyncWebServer.h>

class Router : public AsyncWebHandler
{
public:
  inline Router(Params &params, WiFiManager &wifi)
      : params{params}, wifi{wifi} {}
  virtual ~Router() = default;

  inline bool canHandle(AsyncWebServerRequest *request) override
  {
    return true;
  }

  void handleConnect(AsyncWebServerRequest *request);
  void handleConnInfo(AsyncWebServerRequest *request);
  void handlePin(AsyncWebServerRequest *request, bool &stateVal);
  void handleFree(AsyncWebServerRequest *request);
  void handleConnectAudio(AsyncWebServerRequest *request);
  void handleRequest(AsyncWebServerRequest *request) override;

private:
  Params &params;
  WiFiManager &wifi;
};
