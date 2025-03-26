#include <Arduino.h>
#include <SoftwareSerial.h>
#include <StreamPacket.h>

enum packet_t {
    type1,
    type2,
};

struct S {
    int count;
    uint32_t ms;
    char str[5];
};

SoftwareSerial ss(5, 6);

void setup() {
    Serial.begin(115200);
    ss.begin(9600);
}

void loop() {
    StreamPacket::parse(ss, [](uint8_t type, void* data, size_t len) {
        switch (type) {
            case packet_t::type1: {
                Serial.println("type 1");
                S& s = *((S*)data);
                Serial.println(s.count);
                Serial.println(s.ms);
                Serial.println(s.str);
            } break;

            case packet_t::type2: {
                Serial.println("type 2");
                Serial.write((char*)data, len);
                Serial.println();
            } break;
        }
        Serial.println();
    });
}