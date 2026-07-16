#include <Arduino.h>
#include <WiFi.h>
#include <M5Unified.h>
#include <M5Cardputer.h>
#include <HTTPClient.h>

const char * ssid = "@Hyatt_WiFi";
const char * password = "";
const char * webhook_url = "";
const char * ntp_server = "pool.ntp.org";
const long cst_offset_sec = -6 * 3600; // Central Time Zone (UTC-6) daylight savings. 

const int pir_pin = 1;
bool muted = false;

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

        int httpResponseCode = http.POST("[" + String(timestring) + "] " + message);

        if (httpResponseCode > 0) {
            Serial.printf("Response: %d\n", httpResponseCode);
        } else {
            Serial.printf("Error: %s\n", http.errorToString(httpResponseCode).c_str());
        }
        http.end();
    }
}

void setupTime() {
    configTime(cst_offset_sec, 3600, ntp_server);
}

void keyboard_input() {
    M5Cardputer.update();
    if (M5Cardputer.Keyboard.isChange()) {
        if (M5Cardputer.Keyboard.isKeyPressed('m')) {
            if (muted) {
                M5.Speaker.tone(1000, 200); // Play a tone to indicate unmuting
            } else {
                M5.Speaker.tone(500, 200); // Play a different tone to indicate muting
            }
            muted = !muted;
        }
    }
}

void setup() {
    M5Cardputer.begin(M5.config(), true);
    M5Cardputer.Keyboard.begin();
    // M5.begin();
    M5.Lcd.setBrightness(0);

    Serial.begin(115200);

    Serial.println("Connecting to WiFi...");
    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }


    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nConnected to WiFi!");
        setupTime();
        sendNTFYMessage("PIR Detector is now online!");
    }
    pinMode(pir_pin, INPUT);

    M5.Speaker.tone(1000, 200); // Play a tone to indicate the device has started
}

unsigned long lastMotionTime = 0;
const unsigned long motionDebounceTime = 20 * 1000; // 20 seconds in milliseconds;
bool lastMotionState = LOW;
void loop() {
    keyboard_input();
    if (digitalRead(pir_pin) == HIGH) {
        if (lastMotionState == LOW && millis() - lastMotionTime > motionDebounceTime) { // Check for motion and debounce
            Serial.println("Motion detected!");

            if (!muted) {
                M5.Speaker.tone(2000, 200); // Play a tone to indicate motion detected
            }

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

