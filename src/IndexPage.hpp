#pragma once
#include "Params.hpp"
#include "WiFiManager.hpp"
#include <ESPAsyncWebServer.h>

class IndexPage : public AsyncWebHandler {
public:
  explicit inline IndexPage(Params &params, WiFiManager &wifi)
      : params{params}, wifi{wifi} {}
  virtual inline ~IndexPage() = default;

  bool canHandle(AsyncWebServerRequest *request);
  void handleRequest(AsyncWebServerRequest *request) override;

private:
  Params &params;
  WiFiManager &wifi;
};