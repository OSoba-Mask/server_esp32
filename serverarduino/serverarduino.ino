#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Arduino_JSON.h>

const char* ssid = "OSobaMask";
const char* password = "123406789";

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);
// Create a WebSocket object
AsyncWebSocket ws("/ws");

JSONVar readings;

unsigned long lastTime = 0;
unsigned long timerDelay = 3000;

const int ledPinR = 25;
const int ledPinG = 33;
const int ledPinB = 32;

const int frequency = 5000;
const int redChannel = 0;
const int greenChannel = 1;
const int blueChannel = 2;
const int resolution = 8;

const int ptrPin = 34;
const int ptrPin2 = 35; 

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<p><span id="s1"></span></p>
<p><span id="s2"></span></p>
<p><span id="s3"></span></p>
<input id="slider" type="range" min="0" max="255"/>
<input type="color" id="picker">
<button id="yo">pressme</button>
<script>
let gateway = `ws://${window.location.hostname}/ws`;
let slider = document.querySelector("#slider");
let picker = document.querySelector("#picker");
let websocket;
window.addEventListener('load', onload);

function hexToRgb(hex) {
    let result = /^#?([a-f\d]{2})([a-f\d]{2})([a-f\d]{2})$/i.exec(hex);
    return result ? {
      r: parseInt(result[1], 16),
      g: parseInt(result[2], 16),
      b: parseInt(result[3], 16)
    } : null;
}

function onload(event) {
    initWebSocket();
    initButtons();
}

function initButtons() {
  document.getElementById('yo').addEventListener('click', forwardMove);
}

function forwardMove(){
  //websocket.send('f');

  websocket.send(JSON.stringify(hexToRgb(picker.value)));
}

function getReadings(){
    websocket.send("getReadings");
}

function initWebSocket() {
    console.log('Trying to open a WebSocket connection…');
    websocket = new WebSocket(gateway);
    websocket.onopen = onOpen;
    websocket.onclose = onClose;
    websocket.onmessage = onMessage;
}

function onOpen(event) {
    console.log('Connection opened');
    getReadings();
}

function onClose(event) {
    console.log('Connection closed');
    setTimeout(initWebSocket, 2000);
}

function onMessage(event) {
    console.log(event.data);
    let myObj = JSON.parse(event.data);
    let keys = Object.keys(myObj);

    for (let i = 0; i < keys.length; i++){
        let key = keys[i];
        document.getElementById(key).innerHTML = myObj[key];
    }
}</script>
)rawliteral";


String getSensorReadings(){
  int val = analogRead(ptrPin);
  int val2 = analogRead(ptrPin2);
  Serial.println(val);
  readings["s1"] = String(val);
  readings["s2"] =  String(random(100));
  readings["s3"] = String(random(200));
  Serial.println(val2);
  readings["p1"] = String(val2);
  String jsonString = JSON.stringify(readings);
  return jsonString;
}

void initWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }
  Serial.println(WiFi.localIP());
}

void notifyClients(String sensorReadings) {
  ws.textAll(sensorReadings);
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    
    JSONVar myObject = JSON.parse((const char*)data);
    if (myObject.hasOwnProperty("r")) {      
      int R255 = map((int)myObject["r"], 0, 255, 0, 1023);
      int G255 = map((int)myObject["g"], 0, 255, 0, 1023);
      int B255 = map((int)myObject["b"], 0, 255, 0, 1023);

      ledcWrite(redChannel, 1023 - R255);
      ledcWrite(greenChannel, 1023 - G255);
      ledcWrite(blueChannel, 1023 - B255);
    }

    String sensorReadings = getSensorReadings();
    //Serial.println(sensorReadings);
    notifyClients(sensorReadings);
  }
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
  switch (type) {
    case WS_EVT_CONNECT:
      Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
      break;
    case WS_EVT_DISCONNECT:
      Serial.printf("WebSocket client #%u disconnected\n", client->id());
      break;
    case WS_EVT_DATA:
      handleWebSocketMessage(arg, data, len);
      break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
      break;
  }
}

void initWebSocket() {
  ws.onEvent(onEvent);
  server.addHandler(&ws);
}

void setup() {
  ledcSetup(redChannel, frequency, resolution);
  ledcSetup(greenChannel, frequency, resolution);
  ledcSetup(blueChannel, frequency, resolution);
 
  ledcAttachPin(ledPinR, redChannel);
  ledcAttachPin(ledPinG, greenChannel);
  ledcAttachPin(ledPinB, blueChannel);

  Serial.begin(115200);
  initWiFi();
  initWebSocket();

  // server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
  //   request->send_P(200, "text/html", index_html);
  // });

  server.begin();
}

void loop() {
  //if ((millis() - lastTime) > timerDelay) {
    //String sensorReadings = getSensorReadings();
    //Serial.print(sensorReadings);
    //notifyClients(sensorReadings);
    //lastTime = millis();
  //}
  ws.cleanupClients();
}