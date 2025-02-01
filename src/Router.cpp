#include "Router.hpp"

void Router::handleConnect(AsyncWebServerRequest *request)
{
  auto newSsid = request->arg("SSID");
  auto newPass = request->arg("PASS");
  if (newSsid.isEmpty() || newPass.isEmpty())
  {
    Serial.println("Invalid request!");
    request->send(400, "text/html", "Bad request");
    return;
  }
  if (newSsid == params.ssid && WiFi.status() == WL_CONNECTED)
  {
    Serial.println("Already connected to this AP!");
    return;
  }
  Serial.println("Connecting to:");
  Serial.printf("SSID=%s\n", newSsid.c_str());
  Serial.printf("PASS=%s\n", newPass.c_str());
  params.newSsid = newSsid;
  params.newPass = newPass;
  if (!wifi.connect())
  {
    Serial.printf("Attempting to connect to previous WiFi network: %s\n",
                  params.ssid.c_str());
    request->send(401, "text/html", "Unauthorized");
    wifi.connect();
    return;
  }
  request->redirect("https://" + params.localIp.toString());
}

void Router::handlePin(AsyncWebServerRequest *request, bool &stateVal)
{
  auto state = request->arg("state");
  if (state.isEmpty())
  {
    request->send(200, "application/json",
                  String("{\"state\": ") + (stateVal ? "true" : "false") + "}");
    return;
  }
  if (state == "on")
  {
    stateVal = true;
  }
  else if (state == "off")
  {
    stateVal = false;
  }
  else if (state == "toggle")
  {
    stateVal = !stateVal;
  }
  else
  {
    request->send(400, "text/plain", "Bad Request");
    return;
  }
  request->redirect("/");
}

void Router::handleFree(AsyncWebServerRequest *request)
{
  AsyncResponseStream *response = request->beginResponseStream("text/html", 50);
  auto freeHeap = ESP.getFreeHeap();
  response->printf("Avaliable heap: %u\n", freeHeap);
  response->setCode(200);
  request->send(response);
}

void Router::handleConnInfo(AsyncWebServerRequest *request)
{
  request->send(200, "application/json",
                String("{\"\": ") + (wifi.isConnected() ? "true" : "false") +
                    ",\"network\":\"" + params.ssid + "\"}");
}

void Router::handleConnectAudio(AsyncWebServerRequest *request)
{
  params.remoteIp = request->client()->remoteIP();
  params.dirChange = true;
  request->send(200);
}

void Router::handleRequest(AsyncWebServerRequest *request)
{
  auto const &target = request->url();
  Serial.println(target);
  if (request->url() == "/connect")
  {
    return handleConnect(request);
  }
  if (request->url() == "/conninfo")
  {
    return handleConnInfo(request);
  }
  if (request->url() == "/led")
  {
    return handlePin(request, params.ledEnabled);
  }
  if (request->url() == "/ptt")
  {
    handlePin(request, params.pttEnabled);
    params.dirChange = true;
    return;
  }
  if (request->url() == "/free")
  {
    return handleFree(request);
  }
  if (request->url() == "/connectAudio") {
    return handleConnectAudio(request);
  }
  AsyncResponseStream *response = request->beginResponseStream("text/html");
  response->print("<!DOCTYPE html><html><head><title>ESP "
                  "Radio</title></head><body>");
  response->print("<p>Page not found.</p>");
  response->printf("<p>You were trying to reach: http://%s%s</p>",
                   request->host().c_str(), request->url().c_str());
  response->println("<p>Try opening <a href='/'>this link</a> instead</p>");
  response->print("</body></html>");
  response->setCode(404);
  request->send(response);
}

// void handleClient(WiFiClient client) {
//   bool failedToConnect = false;
//   Serial.println("new client");
//   while (!client.available()) {
//     delay(1);
//   }
//   String request = client.readStringUntil('\r');
//   Serial.println(request);
//   if (request.startsWith("POST /audio")) {
//     if (!client.findUntil("Content-Length: ", "\r\n\r\n")) {
//       Serial.println("No Content-Length header found!");
//       return;
//     }
//     auto contentLengthStr = client.readStringUntil('\r');
//     int contentLength = 0;
//     sscanf(contentLengthStr.c_str(), "%d", &contentLength);
//     if (0 > contentLength || contentLength > bufSize) {
//       Serial.printf("Invalid Content-Length: %d!\n", contentLength);
//       return;
//     }
//     if (!client.find("\r\n\r\n")) {
//       Serial.println("Content not found!");
//       return;
//     }
//     Serial.printf("Received: %d\n", contentLength);
//     auto avail = client.available();
//     while (avail < contentLength) {
//       if (avail < 0) {
//         Serial.println("Stream closed");
//         return;
//       }
//       Serial.printf("Avaliable: %u\n", avail);
//     }
//     auto processed = source->push(
//         [&client](uint8_t *buffer, uint32_t cnt) {
//           Serial.printf("Reading %u bytes\n", cnt);
//           auto ret = client.read(buffer, cnt);
//           // auto ret = client.readBytes(buffer, cnt);
//           Serial.printf("Read %u bytes\n", ret);
//           return ret;
//         },
//         contentLength);
//     Serial.printf("Buffered: %d total=%u\n", processed,
//     source->getFillLevel()); auto missed = contentLength - processed; if
//     (missed) {
//       Serial.printf("Missed audio: %d\n", missed);
//     }

//     if (!mp3->isRunning()) {
//       if (!mp3->begin(source, out)) {
//         Serial.println("Failed to begin mp3!");
//       }
//     }
//     client.print("HTTP/1.1 200 OK\r\n\r\n");
//     client.flush();
//     return;
//   }
// }