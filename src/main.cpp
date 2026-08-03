#include <Arduino.h>
#include <WiFi.h>
#include <M5Unified.h>
#include <M5Cardputer.h>
#include "Messages.hpp"

#include <FastLED.h>

#define NUM_LEDS 1
#define LED_DATA_PIN 21
#define PIR_PIN 2

CRGB leds[NUM_LEDS];

const char * ssid = "@Hyatt_WiFi";
const char * password = "";
const char * webhook_url = ""; // Discord webhook URL for sending messages. Leave empty if not using Discord.
const char * ntp_server = "pool.ntp.org";
const long cst_offset_sec = -6 * 3600; // Central Time Zone (UTC-6) daylight savings. 
float detect_beep_frequency = 2000;

const unsigned long motionDebounceTime = 20 * 1000; // 20 seconds in milliseconds
const unsigned long silenceModeCooldown= 30 * 1000; // 30 seconds in milliseconds
bool lastMotionState = LOW;
bool silenceMode = false;
bool userMuted = false;

uint16_t triggerCount = 0;
uint8_t triggerCountThreshold = 3; // Number of triggers before activating silence mode.
unsigned long lastMotionTime = 0;
unsigned long lastTriggerTime = 0;

const char * regularMessage  = "Motion detected at the front desk!";
const char * silenceModeMessage = "Someone is waiting at the desk.";

Messages messages(webhook_url);

void setupTime() {
    configTime(cst_offset_sec, 3600, ntp_server);
}

void keyboard_input() {
    M5Cardputer.update();
    // Keyboard presses are stored in a buffer until the MCU reads it.
    // I think .isChange() will compare the current buffer contents to it's previous contents.
    if (M5Cardputer.Keyboard.isChange()) {
        if (M5Cardputer.Keyboard.isKeyPressed('m')) {
            if (userMuted) {
                M5.Speaker.tone(1000, 200); // Play a tone to indicate unmuting
            } else {
                M5.Speaker.tone(500, 200); // Play a different tone to indicate muting
            }
            userMuted = !userMuted;
        }

        if (M5Cardputer.Keyboard.isKeyPressed('1')) {
            detect_beep_frequency = detect_beep_frequency - 100;
        }
        else if (M5Cardputer.Keyboard.isKeyPressed('2')) {
            detect_beep_frequency = detect_beep_frequency + 100;
        }

        if (M5Cardputer.Keyboard.isKeyPressed('t')) {
            messages.sendNTFYMessage("Test message");
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
        messages.sendNTFYMessage("PIR Detector is now online!");
    }
    pinMode(PIR_PIN, INPUT_PULLDOWN); // HC-SR501 outputs HIGH when motion is detected.

    messages.sendNTFYMessage("PIR device warming for 1 minute...");
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
    messages.sendNTFYMessage("PIR device is now ready!");
    leds[0] = CRGB::Green; // Set LED to green to indicate ready
    FastLED.show();

    M5.Speaker.tone(1000, 200); // Play a tone to indicate the device has started
}

void updateLED() {
    if (silenceMode) {
        leds[0] = CRGB::Blue; // Set LED to blue to indicate silence mode
    } else {
        leds[0] = CRGB::Green; // Set LED to green to indicate normal operation
    }
    FastLED.show();
}

void executeAlert() {
    if (!userMuted && !silenceMode) {
        M5.Speaker.tone(detect_beep_frequency, 200); // Play a tone to indicate motion detected
    }

    // sendDiscordMessage("Motion Detected at the front desk!");
    messages.sendNTFYMessage(silenceMode ? silenceModeMessage : regularMessage);
}

void updateTimerAndState() {
    if (millis() - lastTriggerTime > silenceModeCooldown) {
        // Silence mode is active. Check if the cooldown period has passed to exit silence mode.
        if (silenceMode == true) {
            silenceMode = false;
            leds[0] = CRGB::Green;
            FastLED.show();
        }

        // Reset the trigger count after the cooldown period has passed. whether silence mode is active or not.
        triggerCount = 0;
    }
}

void handleValidTrigger() {
    triggerCount++;
    lastTriggerTime = millis();
    lastMotionTime = millis();
    lastMotionState = HIGH;


    if (triggerCount == triggerCountThreshold) { // Activate silence mode
        userMuted = true;
        silenceMode = true;
        leds[0] = CRGB::Blue;
        FastLED.show();
    }
    
    executeAlert();
}

void checkSensor() {
    if (digitalRead(PIR_PIN) == HIGH) {
        // Keep silence mode on if motion is detected during the cooldown period.
        // This check is independent of the motion debounce time. 
        // The final reading may be outside the debounce time, avoiding an unnecessary motion debounce time added to the cooldown period.
        if (silenceMode == true) {
            lastTriggerTime = millis(); // Reset the cooldown timer if motion is detected during silence mode
        }

        if (lastMotionState == LOW && millis() - lastMotionTime > motionDebounceTime) {
            handleValidTrigger();
        }
    } else {
        lastMotionState = LOW;
    }
}

void loop() {
    keyboard_input();
    updateTimerAndState();
    checkSensor();
}

