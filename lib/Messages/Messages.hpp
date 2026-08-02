#pragma once

#include <Arduino.h>

class Messages {
    private:
        const char * webhook_url;

    public:
        Messages(const char * url);
        void sendDiscordMessage(char * message);
        void sendNTFYMessage(String message);
};