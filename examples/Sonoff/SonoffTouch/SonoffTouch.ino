#define THINGER_DEVICE_SSID "SonoffTouch"
#define THINGER_DEVICE_SSID_PSWD "SonoffTouch"
#define _DEBUG_

// Requires WifiManager from Library Manager or https://github.com/tzapu/WiFiManager
#include <ThingerWebConfig.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <Ticker.h>

#define BUTTON_PIN 0
#define RELAY_PIN 12
#define LED_PIN 13

ThingerWebConfig thing;
Ticker ticker;

unsigned long millisSinceChange = 0;

void toggleWiFiStatus(){
  digitalWrite(LED_PIN, !digitalRead(LED_PIN));
}

void setup() {
  Serial.begin(115200);

  pinMode(RELAY_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);

  digitalWrite(RELAY_PIN, LOW); // LIGH off
  digitalWrite(LED_PIN, HIGH); //LED off.

  // define thinger resources for the touch
  thing["light"] << digitalPin(RELAY_PIN);

  // atach interrupt to detect switch changes
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), buttonChangeCallback, CHANGE);

  // set OTA hostname
  char hostname[15];
  sprintf(hostname, "sonoff-%06x", ESP.getChipId());
  ArduinoOTA.setHostname(hostname);

  // set a basic OTA password
  ArduinoOTA.setPassword("sonoff");

  // show progress while upgrading flash
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    toggleWiFiStatus();
  });

  // init arduino OTA
  ArduinoOTA.begin();

  ticker.attach(1, toggleWiFiStatus);
}

void buttonChangeCallback() {
  unsigned long currentMillis = millis();
  if (digitalRead(BUTTON_PIN) == 1) {
    // >20s, clean both thinger and wifi information
    if (currentMillis - millisSinceChange > 20000) {
      WiFi.disconnect();
      thing.clean_credentials();
      ESP.reset();
    // >10s, clean only wifi information
    }else if(currentMillis - millisSinceChange > 10000){
      WiFi.disconnect();
      ESP.reset();
    }
    // else, toggle relay
    else if (currentMillis - millisSinceChange > 100) {
      digitalWrite(RELAY_PIN, !digitalRead(RELAY_PIN));
    }
  }
  millisSinceChange = currentMillis;
}

void loop() {
  // handle connection
  thing.handle();

  // keep light on while connected
  digitalWrite(LED_PIN, LOW); //LED ON on WifiConnect

  // handle OTA
  ArduinoOTA.handle();
}