
#include <WiFi.h>
#include <HTTPClient.h>
#include "Messages.hpp"
#include <M5Cardputer.h>

Messages::Messages(const char * url) : webhook_url(url) {
    // Constructor implementation
}

void Messages::sendDiscordMessage(char * message) { 
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

void Messages::sendNTFYMessage(String message) {
    struct tm timeinfo;
    char timestring[64];
    if (!getLocalTime(&timeinfo)) {
        Serial.println("Failed to obtain time");
        strcpy(timestring, "Unknown Time");
    } else {
        strftime(timestring, sizeof(timestring), "%H:%M:%S", &timeinfo);
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("Sending message to ntfy...");
        HTTPClient http;
        http.begin("https://ntfy.sh/CZMK-PIR-Detector");
        http.addHeader("Content-Type", "text/plain");

        int httpResponseCode = http.POST("[" + String(timestring) + "] [" + String(M5Cardputer.Power.getBatteryLevel()) + "%]" + message);

        if (httpResponseCode > 0) {
            Serial.printf("Response: %d\n", httpResponseCode);
        } else {
            Serial.printf("Error: %s\n", http.errorToString(httpResponseCode).c_str());
        }
        http.end();
    }
}