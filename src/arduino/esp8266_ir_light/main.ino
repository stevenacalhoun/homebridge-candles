#include <ESP8266WiFi.h>
#include <IRremote.hpp>

#include "ir_defines.h"
#include "secret_defines.h"

// Heartbeat setup
#define HEARTBEAT_PERIOD 500
int heartbeatStatus = 1;
unsigned long heartbeatLast = 0;

// Local homebridge address
#define HOMEBRIDGE "homebridge.local"

// Button control pins
#define ON_BUTTON_PIN 14
#define OFF_BUTTON_PIN 12

// ESP8266 on-board LED is pull down
#define LED_ON LOW
#define LED_OFF HIGH

// Wifi setup
WiFiServer server(80);

String accessoryId;

void setup() {
  // Setup serial connection
  Serial.begin(74880);
  Serial.println("init");

  // Configure heart beat and on/off pins
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(ON_BUTTON_PIN, INPUT_PULLUP);
  pinMode(OFF_BUTTON_PIN, INPUT_PULLUP);

  // Start wifi and IR
  wifiConnect();
  IrSender.begin(IR_TX_PIN);

  // Get accessory ID
  // TODO should this fail, we should implement some sort of retry
  accessoryId = fetchAccessoryId();
}

void wifiConnect() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(SECRET_SSID, SECRET_PSWD);

  Serial.print("Connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  Serial.print("Connected, IP address: ");
  Serial.println(WiFi.localIP());
  Serial.print("Mac address: ");
  Serial.println(WiFi.macAddress());
  server.begin();
}

String fetchAccessoryId() {
  // TODO I'm sure this string concat doesn't work
  return fetchUrl(HOMEBRIDGE + "/id/" + WiFi.macAddress());
}

void loop() {
  heartbeat();
  checkButtons();

  // Pull strategy
  fetchStatus();

  // Push strategy
  /* checkWebServer(); */
}

void heartbeat() {
  unsigned long currentTime = millis();
  signed long timeDiff = currentTime - heartbeatLast;
  if (abs(timeDiff) > HEARTBEAT_PERIOD) {
    heartbeatLast = millis();
    heartbeatStatus = !heartbeatStatus;
    digitalWrite(LED_BUILTIN, heartbeatStatus);
  }
}

void checkButtons() {
  if (onButtonValue()) {
    Serial.println("On button");
    turnCandlesOn();
  }

  if (offButtonPushed()) {
    Serial.println("Off button");
    turnCandlesOff();
  }
}

bool onButtonValue() {
  return !digitalRead(ON_BUTTON_PIN);
}

bool offButtonPushed() {
  return !digitalRead(OFF_BUTTON_PIN);
}

void fetchStatus() {
  // Pull status from homebridge
  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient https;

  // TODO I'm sure this string concat doesn't work
  String data = fetchUrl(HOMEBRIDGE + "/status/" + accessoryId)

  // Switch lights
  if (data == "true") {
    turnCandlesOn();
  }
  else {
    turnCandlesOff();
  }
}

String fetchUrl(String url) {
  // TODO never run...got test it
  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient https;

  Serial.println("Requesting " + url);

  if (https.begin(client, url)) {
    int httpCode = https.GET();
    Serial.println("============== Response code: " + String(httpCode));
    if (httpCode > 0) {
      String data = https.getString();
      Serial.print("Current Status: ");
      Serial.println(data);
    }
    https.end();
  } else {
    Serial.println("[HTTPS] Unable to connect");
  }

  return data
}

void checkWebServer() {
  // Check if a client has connected
  WiFiClient client = server.accept();
  if (!client) { return; }
  Serial.println(F("new client"));

  client.setTimeout(5000);

  // Read the first line of the request
  String req = client.readStringUntil('\r');
  Serial.println(F("request: "));
  Serial.println(req);

  // Match the request
  int val;
  String resp;
  if (req.indexOf(F("/on")) != -1) {
    turnCandlesOn();
    resp = "Candles On";
  }
  else if (req.indexOf(F("/off")) != -1) {
    turnCandlesOff();
    resp = "Candles Off";
  }
  else if (req.indexOf(F("/status")) != -1) {
    resp = "Candles Alive";
  }
  else {
    resp = "Invalid request";
  }

  // Read rest of request
  while (client.available()) {
    client.read();
  }

  // Send response
  client.print(F("HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\n"));
  client.print(resp);
}

void turnCandlesOn() {
  IrSender.sendNEC(IR_ADDRESS, CANDLES_ON, REPEATS);
}

void turnCandlesOff() {
  IrSender.sendNEC(IR_ADDRESS, CANDLES_OFF, REPEATS);
}
