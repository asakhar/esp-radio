#include <Arduino.h>
#include <AudioFileSourceBuffer.h>
#include <AudioFileSourceFeed.hpp>
#include <AudioFileSourceHTTPStream.h>
#include <AudioGeneratorMP3a.h>
#include <AudioOutputI2SNoDAC.h>
#include <ESP8266WiFi.h>

// const char *ssid = "HUAWEI-B593-AC9B"; // your WiFi Name
// const char *password = "5G7LE1EH7F5";  // Your Wifi Password
String ssid = "Rabbit";        // your WiFi Name
String pass = "QwPoAsLk01#73"; // Your Wifi Password
const char *apssid = "ESP8266 Radio";
const char *appassword = "QwPoAsLk01#73";
int ledPin = 2;
int pttPin = 16;
int ledValue = LOW;
int pttValue = HIGH;
WiFiServer server(80);
IPAddress localIp;
IPAddress apIp;
const int bufSize = 6144;

AudioGeneratorMP3a *mp3;
AudioOutputI2SNoDAC *out;
AudioFileSourceFeed *source;

String decodePecent(String in) {
  String res;
  int p = 0;
  int i = in.indexOf('%');
  for (; i != -1 && i + 1 < static_cast<int>(in.length());
       p = i, i = in.indexOf('%', i)) {
    res += in.substring(p, i);
    if (in[i + 1] == '%') {
      res += '%';
      i += 2;
      continue;
    }

    if (isHexadecimalDigit(in[i + 1]) && isHexadecimalDigit(in[i + 2])) {
      auto hexNum = in.substring(i + 1, i + 3);
      char hex = strtoul(hexNum.c_str(), NULL, 16);
      res += hex;
      i += 3;
      continue;
    }
    i += 1;
    res += '%';
  }
  if (p < static_cast<int>(in.length())) {
    res += in.substring(p);
  }
  return res;
}

int connectToWiFi(String const &newssid, String const &newpass) {
  int attempts = 0;
  Serial.print("Connecting to ");
  Serial.println(newssid);
  WiFi.begin(newssid, newpass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    if (attempts++ > 20) {
      return -1;
    }
  }
  ssid = newssid;
  pass = newpass;
  Serial.println("");
  Serial.print("WiFi connected to ");
  Serial.print(newssid);
  Serial.println("!");
  localIp = WiFi.localIP();
  Serial.print("Use this URL to connect: ");
  Serial.print("http://");
  Serial.print(localIp);
  Serial.println("/");
  return 0;
}

int connectToWiFi() { return connectToWiFi(ssid, pass); }

void setup() {
  Serial.begin(115200);
  audioLogger = &Serial;
  source = new AudioFileSourceFeed(bufSize);
  out = new AudioOutputI2SNoDAC();
  mp3 = new AudioGeneratorMP3a();

  int connectionStatus = 0;
  pinMode(ledPin, OUTPUT);
  pinMode(pttPin, OUTPUT);
  digitalWrite(ledPin, ledValue);
  digitalWrite(pttPin, pttValue);
  delay(10);
  Serial.println();
  Serial.println();
  WiFi.mode(WIFI_AP_STA);
  if (!WiFi.softAP(apssid, appassword)) {
    Serial.println("ERROR: Soft AP creation failed.");
    while (1)
      ;
  }
  apIp = WiFi.softAPIP(); // IP address for AP mode

  connectionStatus = connectToWiFi();

  Serial.print("AP IP address: ");
  Serial.println(apIp);

  server.begin();
  Serial.println("Server started");
  Serial.print("Use this URL to connect: ");
  Serial.print("http://");
  Serial.print(apIp);
  Serial.println("/");
  if (connectionStatus == 0) {
    Serial.println("or:");
    Serial.print("http://");
    Serial.print(localIp);
    Serial.println("/");
  }
}

void handleClient(WiFiClient client) {
  bool failedToConnect = false;
  Serial.println("new client");
  while (!client.available()) {
    delay(1);
  }
  String request = client.readStringUntil('\r');
  Serial.println(request);
  if (request.startsWith("POST /audio")) {
    if (!client.findUntil("Content-Length: ", "\r\n\r\n")) {
      Serial.println("No Content-Length header found!");
      return;
    }
    auto contentLengthStr = client.readStringUntil('\r');
    int contentLength = 0;
    sscanf(contentLengthStr.c_str(), "%d", &contentLength);
    if (0 > contentLength || contentLength > bufSize) {
      Serial.printf("Invalid Content-Length: %d!\n", contentLength);
      return;
    }
    if(!client.find("\r\n\r\n")) {
      Serial.println("Content not found!");
      return;
    }
    Serial.printf("Received: %d\n", contentLength);
    auto avail = client.available();
    while(avail < contentLength) {
      if (avail < 0) {
        Serial.println("Stream closed");
        return;
      }
      Serial.printf("Avaliable: %u\n", avail);
    }
    auto processed = source->push(
        [&client](uint8_t *buffer, uint32_t cnt) {
          Serial.printf("Reading %u bytes\n", cnt);
          auto ret = client.read(buffer, cnt);
          // auto ret = client.readBytes(buffer, cnt);
          Serial.printf("Read %u bytes\n", ret);
          return ret;
        },
        contentLength);
    Serial.printf("Buffered: %d total=%u\n", processed, source->getFillLevel());
    auto missed = contentLength - processed;
    if (missed) {
      Serial.printf("Missed audio: %d\n", missed);
    }

    if (!mp3->isRunning()) {
      if (!mp3->begin(source, out)) {
        Serial.println("Failed to begin mp3!");
      }
    }
    client.print("HTTP/1.1 200 OK\r\n\r\n");
    client.flush();
    return;
  }
  client.flush();
  if (request.startsWith("GET /connect?")) {
    auto query_start = request.indexOf("?");
    auto query_end = request.indexOf(" ", query_start);
    auto query = request.substring(query_start + 1, query_end);
    auto ssid_start = query.indexOf("SSID=");
    auto pass_start = query.indexOf("PASS=");
    auto ssid_end = query.indexOf("&", ssid_start);
    auto pass_end = query.indexOf("&", pass_start);
    if (ssid_start == -1 || pass_start == -1) {
      Serial.println("Invalid request!");
      client.println("HTTP/1.1 400 Bad request");
      client.println();
      client.println();
      return;
    }
    if (ssid_end == -1) {
      ssid_end = query.length();
    }
    if (pass_end == -1) {
      pass_end = query.length();
    }
    auto newSsid = decodePecent(query.substring(ssid_start + 5, ssid_end));
    auto newPass = decodePecent(query.substring(pass_start + 5, pass_end));
    if (newSsid == ssid && WiFi.status() == WL_CONNECTED) {
      Serial.println("Already connected to this AP!");
    } else {
      Serial.println("Connecting to:");
      Serial.print("SSID=");
      Serial.println(newSsid);
      Serial.print("PASS=");
      Serial.println(newPass);
      if (connectToWiFi(newSsid, newPass) != 0) {
        failedToConnect = true;
        connectToWiFi();
      }
    }
  }
  if (request.indexOf("/LED=ON") != -1) {
    ledValue = LOW;
  }
  if (request.indexOf("/LED=OFF") != -1) {
    ledValue = HIGH;
  }
  digitalWrite(ledPin, ledValue);
  if (request.indexOf("/PTT=ON") != -1) {
    pttValue = LOW;
  }
  if (request.indexOf("/PTT=OFF") != -1) {
    pttValue = HIGH;
  }
  digitalWrite(pttPin, pttValue);
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html");
  client.println("");
  client.println("");
  client.println("");
  client.println(R"##(
<!DOCTYPE html>
<html lang="en">

<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Document</title>
</head>

<body>
<button id="btn-play" onclick="onPlayClick()">Play</button>
<script>
/**
 * @type MediaRecorder
 */
var recorder;
/**
 * @type HTMLAudioElement
 */
var audio;
var isPaused = true;

/**
 * 
 * @param {BlobEvent} ev 
 */
async function onDataAvailable(ev) {
  console.log(ev.data);
  var data = await ev.data.arrayBuffer();
  for (let i = 0; i < Math.ceil(data.byteLength / 1536); ++i) {
    let tmp = data.slice(i * 1536, Math.min((i + 1) * 1536, data.byteLength));
    fetch(`http://${window.location.hostname}/audio`, {
      method: "POST",
      body: tmp
    });
  }
}

async function onPlayClick() {
  if (recorder) {
    if (isPaused) {
      recorder.resume();
    } else {
      recorder.pause();
    }
    return;
  }
  const stream = await navigator.mediaDevices.getUserMedia({ audio: true });
  recorder = new MediaRecorder(stream, {
    mimeType: 'audio/mp4',
    audioBitsPerSecond: 32000,
    videoBitsPerSecond: 0,
    audioBitrateMode: 'constant'
  });
  recorder.ondataavailable = onDataAvailable;
  recorder.onpause = () => isPaused = true;
  recorder.onresume = () => isPaused = false;
  recorder.start(20);
  recorder.pause();
}
</script>
  )##");
  if (failedToConnect) {
    client.print("Failed to connect to ");
    client.print(ssid);
    client.println("!<br/>");
  }
  if (!ssid.isEmpty()) {
    client.print("Connected to ");
    client.print(ssid);
    client.println("!<br/>");
  }
  client.println("Connect to WiFi:");
  client.println(R"(<form action="connect">
    <label for="ssid">WiFi SSID:</label>
    <input type="text" name="SSID" id="ssid">
    <br />
    <label for="pass">WiFi password:</label>
    <input type="password" name="PASS" id="pass">
    <br />
    <input type="submit" value="Connect">
  </form><br/><br/>)");
  client.print("Led is: ");
  if (ledValue == LOW) {
    client.print("On");
  } else {
    client.print("Off");
  }
  client.println("<br/>");
  client.print("PTT is: ");
  if (pttValue == LOW) {
    client.print("On");
  } else {
    client.print("Off");
  }
  client.println("<br/>");
  client.print(" <a href=\"/LED=");
  client.print(ledValue == HIGH ? "ON" : "OFF");
  client.println("\">Turn ");
  client.print(ledValue == HIGH ? "on" : "off");
  client.print(" LED</a><br/>");
  client.print(" <a href=\"/PTT=");
  client.print(pttValue == HIGH ? "ON" : "OFF");
  client.println("\">Turn ");
  client.print(pttValue == HIGH ? "on" : "off");
  client.print(" PTT</a><br/>");
  client.println("</body></html>");
  client.flush();
  delay(1);
  Serial.println("Client disonnected");
  Serial.println("");
}

void loop() {
  WiFiClient client = server.accept();
  if (client) {
    handleClient(client);
  }
  if (mp3->isRunning()) {
    if (!mp3->loop())
      mp3->stop();
  }
}
