#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266HTTPClient.h>

#include <WiFiClient.h>
#include <IRremote.hpp>

#include "ir_candle_defines.h"
#include "secret_defines.h"

// Pin Settings
#define IR_TX_PIN 13

// Heartbeat setup
#define HEARTBEAT_PERIOD 500
int heartbeatStatus = 1;
unsigned long heartbeatLast = 0;

// Local homebridge address
#define HOMEBRIDGE "http://homebridge.local:18081"
#define FETCH_PERIOD 3000
String accessoryId;
int currentLightStatus = -1;
unsigned long lastLightFetchTime = 0;
unsigned long lastButtonPushTime = 0;

// Button control pins
#define ON_BUTTON_PIN 14
#define OFF_BUTTON_PIN 12

// ESP8266 on-board LED is pull down
#define LED_ON LOW
#define LED_OFF HIGH

// Wifi setup
WiFiClient client;
ESP8266WiFiMulti WiFiMulti;

// #define DEBUG

void setup() {
  // Setup serial connection
  #ifdef DEBUG
    Serial.begin(74880);
  #endif
  Serial.println("init");

  // Configure heart beat and on/off pins
  #ifdef DEBUG
    pinMode(LED_BUILTIN, OUTPUT);
  #endif
  pinMode(ON_BUTTON_PIN, INPUT_PULLUP);
  pinMode(OFF_BUTTON_PIN, INPUT_PULLUP);

  // Start wifi
  WiFi.hostname("ESP-host");
  WiFi.mode(WIFI_STA);
  WiFiMulti.addAP(SECRET_SSID, SECRET_PSWD);

  // Start IR
  IrSender.begin(IR_TX_PIN);
}

String fetchAccessoryId() {
  String url = HOMEBRIDGE;
  url += "/id/";
  url += WiFi.macAddress();
  return fetchUrl(url);
}

void loop() {
  #ifdef DEBUG
    heartbeat();
  #endif

  checkButtons();

  // Pull strategy
  pullStatus();  
}

void heartbeat() {
  unsigned long currentTime = millis();
  signed long timeDiff = currentTime - heartbeatLast;
  if (abs(timeDiff) > HEARTBEAT_PERIOD) {
    heartbeatLast = currentTime;
    heartbeatStatus = !heartbeatStatus;
    digitalWrite(LED_BUILTIN, heartbeatStatus);
  }
}

void pullStatus() {
  // Only fetch if it is time to do so
  unsigned long currentTime = millis();
  signed long timeDiff = currentTime - lastLightFetchTime;
  if (abs(timeDiff) < FETCH_PERIOD) { return; }

  //  If ID has not been set, fetch it
  if (accessoryId == "") { accessoryId = fetchAccessoryId(); }

  // If it still hasn't been set, don't fetch status
  if (accessoryId == "") { return; }

  // Fetch status url
  String url = HOMEBRIDGE;
  url += "/status/";
  url += accessoryId;
  url += "?currentLightStatus=";
  url += currentLightStatus ? "true" : "false";
  url += "&lastButtonPushTime=";
  url += lastButtonPushTime;
  url += "";
  String data = fetchUrl(url);

  // Parse response data
  int newStatus = (data == "true") ? 1 : 0;

  // If we have a new status, change the lights
  if (newStatus != currentLightStatus) {
    currentLightStatus = newStatus;
    currentLightStatus ? turnOn() : turnOff();
  }

  lastLightFetchTime = currentTime;
}

String fetchUrl(String url) {
  #ifdef DEBUG
    Serial.println("GET " + url);
  #endif
  String data = "";

  if ((WiFiMulti.run() == WL_CONNECTED)) {
    HTTPClient http;

    if (http.begin(client, url)) {
      int httpCode = http.GET();
      if (httpCode > 0) {
        data = http.getString();
        #ifdef DEBUG
          Serial.print("Response: ");
          Serial.println(data);
        #endif
      } 
      else {
        Serial.println("Response code: " + String(httpCode));
        Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
      }
      http.end();
    } 
    else {
      Serial.println("[HTTP] Unable to connect");
    }
  }
  else {
    Serial.println("WiFi multi never got connected");
  }

  return data;
}

void turnOn() {
  Serial.println("Turning on");
  IrSender.sendNEC(IR_ADDRESS, LIGHT_ON, REPEATS);
}

void turnOff() {
  Serial.println("Turning off");
  IrSender.sendNEC(IR_ADDRESS, LIGHT_OFF, REPEATS);
}

void checkButtons() {
  if (onButtonValue()) {
    Serial.println("On button");
    turnOn();
    currentLightStatus = 1;
    lastButtonPushTime = millis();
  }

  if (offButtonPushed()) {
    Serial.println("Off button");
    turnOff();
    currentLightStatus = 0;
    lastButtonPushTime = millis();
  }
}

bool onButtonValue() {
  return !digitalRead(ON_BUTTON_PIN);
}

bool offButtonPushed() {
  return !digitalRead(OFF_BUTTON_PIN);
}

/* Push functions
void startServer() {
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
*/