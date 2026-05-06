#include <Arduino.h>
#include <WiFi.h>
#include <M5Unified.h>
#include <HTTPClient.h>

const char * ssid = "@Hyatt_WiFi";
const char * password = "";
const char * webhook_url = "https://discord.com/api/webhooks/1501099874760790098/U4ahp5mJPrceyyiFp6LIe9HZcXKx8sL8oqmMCXE_YnR7pQzZSm70emhTVr0BrVD6widh";

const int pir_pin = 1;

void sendDiscordMessage(char * message) { 
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("Sending message to Discord...");
        HTTPClient http;
        http.begin(webhook_url);
        http.addHeader("Content-Type", "application/json");

        // Discord expects a JSON payload with a "content" field
        String jsonPayload = "{\"content\": \"" + String(message) + "\"}";
        int httpResponseCode = http.POST(jsonPayload);

        if (httpResponseCode > 0) {
            Serial.printf("Response: %d\n", httpResponseCode);
        } else {
            Serial.printf("Error: %s\n", http.errorToString(httpResponseCode).c_str());
        }
        http.end();
    }
}

void sendNTFYMessage(String message) {
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("Sending message to ntfy...");
        HTTPClient http;
        http.begin("https://ntfy.sh/CZMK-PIR-Detector");
        http.addHeader("Content-Type", "text/plain");

        int httpResponseCode = http.POST(message);

        if (httpResponseCode > 0) {
            Serial.printf("Response: %d\n", httpResponseCode);
        } else {
            Serial.printf("Error: %s\n", http.errorToString(httpResponseCode).c_str());
        }
        http.end();
    }
}

void setup() {
  M5.begin();
  Serial.begin(115200);
  delay(3000);
  Serial.println("Connecting to WiFi...");
    WiFi.begin(ssid, password);
    
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.println("\nConnected to WiFi!");
    if (WiFi.status() == WL_CONNECTED) {
        sendNTFYMessage("PIR Detector is now online!");
    }
    pinMode(pir_pin, INPUT);
}

unsigned long lastMotionTime = 0;
const unsigned long motionDebounceTime = 20 * 1000; // 20 seconds in milliseconds;
bool lastMotionState = LOW;
void loop() {
  if (digitalRead(pir_pin) == HIGH) {
    if (lastMotionState == LOW && millis() - lastMotionTime > motionDebounceTime) { // Check for motion and debounce
        Serial.println("Motion detected!");
        lastMotionTime = millis();
        lastMotionState = HIGH;
        // sendDiscordMessage("Motion Detected at the front desk!");
        sendNTFYMessage("Motion Detected at the front desk!");
        delay(1000); // Debounce delay
    }
  }
  else {
    lastMotionState = LOW;
  }
}

