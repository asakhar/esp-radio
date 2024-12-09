#include "IndexPage.hpp"

bool IndexPage::canHandle(AsyncWebServerRequest *request) {
  return request->url() == "/";
}

void IndexPage::handleRequest(AsyncWebServerRequest *request) {
  Serial.printf("Handling index: %s\n", request->url().c_str());
  AsyncResponseStream *response = request->beginResponseStream("text/html");
  response->print(R"##(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>ESP Radio</title>
  <script src="main.js"></script>
</head>
<body>
<audio id="audio" autoplay="autoplay" controls="controls"></audio><br/>
<button id="btn-record">Record</button><br/>
<button id="btn-stop">Stop</button><br/>
<button id="btn-play">Play</button><br/>
<h>Connect to WiFi:</h>
<form action="connect">
  <label for="ssid">WiFi SSID:</label>
  <input type="text" name="SSID" id="ssid">
  <br />
  <label for="pass">WiFi password:</label>
  <input type="password" name="PASS" id="pass">
  <br />
  <input type="submit" value="Connect">
</form><br/><br/>
)##");
  if (auto newSsid = params.newSsid) {
    response->printf("Failed to connect to '%s'!<br/>\n", newSsid->c_str());
  }
  if (!wifi.isConnected()) {
    response->printf("Connected to '%s'!<br/>\n", params.ssid.c_str());
  }
  auto ledStatus = params.ledEnabled ? "On" : "Off";
  auto pttStatus = params.pttEnabled ? "On" : "Off";
  auto ledAction = params.ledEnabled ? "off" : "on";
  auto pttAction = params.pttEnabled ? "off" : "on";
  response->printf("Led is: %s\n<br/>\n", ledStatus);
  response->printf("PTT is: %s\n<br/>\n", pttStatus);
  response->printf("<a href=\"/led?state=%s\">Turn %s LED<a/><br/>\n",
                   ledAction, ledAction);
  response->printf("<a href=\"/ptt?state=%s\">Turn %s PTT<a/><br/>\n",
                   pttAction, pttAction);
  response->println("</body></html>");
  response->setCode(200);
  request->send(response);
}