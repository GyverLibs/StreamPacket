#include <Arduino.h>
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

void setup() {
    Serial.begin(9600);
}

void loop() {
    static uint32_t t1, t2;

    if (millis() - t1 >= 500) {
        t1 = millis();

        static S s{0, 0, "abcd"};
        s.count++;
        s.ms = millis();
        StreamPacket::send(Serial, packet_t::type1, s);
    }

    if (millis() - t2 >= 2000) {
        t2 = millis();

        String str;
        str += "uptime: ";
        str += millis() / 1000;
        StreamPacket::send(Serial, packet_t::type2, str.c_str(), str.length());
    }
}