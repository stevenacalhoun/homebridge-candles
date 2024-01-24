#include <ESP8266WiFi.h>
#include <IRremote.hpp>

#include "IR_defines.h"
#include "secret_defines.h"

#define LED_ON LOW
#define LED_OFF HIGH

// Heartbeat setup
#define HEARTBEAT_PERIOD 500
int heartbeatStatus = 1;
unsigned long heartbeatLast = 0;

// Wifi setup
WiFiServer server(80);

void setup() {
  Serial.begin(74880);
  Serial.println("init");

  // Configure heart beat and on/off pins
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(ON_BUTTON_PIN, INPUT_PULLUP);
  pinMode(OFF_BUTTON_PIN, INPUT_PULLUP);

  // Start wifi and IR
  wifiConnect();
  IrSender.begin(IR_TX_PIN);
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

void loop() {
  //tripleBlink();
  heartbeat();
  checkWebServer();
  checkButtons();
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
  if (onButtonPushed()) {
    Serial.println("On button");
    turnCandlesOn();
  }

  if (offButtonPushed()) {
    Serial.println("Off button");
    turnCandlesOff();
  }
}

bool onButtonPushed() {
  return !digitalRead(ON_BUTTON_PIN);
}

bool offButtonPushed() {
  return !digitalRead(OFF_BUTTON_PIN);
}

void getStatus() {
  
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
  IrSender.sendNEC(CANDLE_ADDRESS, CMD_ON, REPEATS);
}

void turnCandlesOff() {
  IrSender.sendNEC(CANDLE_ADDRESS, CMD_OFF, REPEATS);
}
