#include <Arduino.h>
#include <WiFi.h>
#include <M5Unified.h>
#include <M5Cardputer.h>
#include <HTTPClient.h>
#include <FastLED.h>

#define NUM_LEDS 1
#define LED_DATA_PIN 21

CRGB leds[NUM_LEDS];

const char * ssid = "@Hyatt_WiFi";
const char * password = "";
const char * webhook_url = ""; // Discord webhook URL for sending messages. Leave empty if not using Discord.
const char * ntp_server = "pool.ntp.org";
const long cst_offset_sec = -6 * 3600; // Central Time Zone (UTC-6) daylight savings. 
float detect_beep_frequency = 2000;

const int pir_pin = 2;
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

        int httpResponseCode = http.POST("[" + String(timestring) + "] [" + String(M5Cardputer.Power.getBatteryLevel()) + "%]" + message);

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

        if (M5Cardputer.Keyboard.isKeyPressed('1')) {
            detect_beep_frequency = detect_beep_frequency - 100;
        }
        else if (M5Cardputer.Keyboard.isKeyPressed('2')) {
            detect_beep_frequency = detect_beep_frequency + 100;
        }
    }
}

void setup() {
    M5Cardputer.begin(M5.config(), true);
    M5Cardputer.Keyboard.begin();
    // M5.begin();
    M5.Lcd.setBrightness(0);

    FastLED.addLeds<WS2812, LED_DATA_PIN, GRB>(leds, NUM_LEDS);

    Serial.begin(115200);

    Serial.println("Connecting to WiFi...");
    WiFi.begin(ssid, password);

    // Program will be stuck here until connected to WiFi
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    // Don't really need to check if I keep the above loop... Might change this later.
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nConnected to WiFi!");
        setupTime();
        sendNTFYMessage("PIR Detector is now online!");
    }
    pinMode(pir_pin, INPUT_PULLDOWN); // HC-SR501 outputs HIGH when motion is detected.

    sendNTFYMessage("PIR device warming for 1 minute...");
    leds[0] = CRGB::Red; // Set LED to red to indicate warming up
    FastLED.show();
    int start_time = millis();
    // Wait for 1 minute to allow the PIR sensor to stabilize
    while (millis() - start_time < 60000) {
        M5Cardputer.update();
        if (M5Cardputer.BtnA.wasPressed()) {
            // skip PIR warm up.
            break;
        }
    }
    sendNTFYMessage("PIR device is now ready!");
    leds[0] = CRGB::Green; // Set LED to green to indicate ready
    FastLED.show();

    M5.Speaker.tone(1000, 200); // Play a tone to indicate the device has started
}

unsigned long lastMotionTime = 0;
const unsigned long motionDebounceTime = 20 * 1000; // 20 seconds in milliseconds;
bool lastMotionState = LOW;
void loop() {
    keyboard_input();
    if (digitalRead(pir_pin) == HIGH) {
        Serial.println(M5Cardputer.Power.getBatteryLevel());
        if (lastMotionState == LOW && millis() - lastMotionTime > motionDebounceTime) { // Check for motion and debounce
            Serial.println("Motion detected!");

            if (!muted) {
                M5.Speaker.tone(detect_beep_frequency, 200); // Play a tone to indicate motion detected
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

