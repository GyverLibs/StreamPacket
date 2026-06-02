This is an automatic translation and may be incorrect in some places. See the source README and examples for authoritative information.

[![latest](https://img.shields.io/github/v/release/GyverLibs/StreamPacket.svg?color=brightgreen)](https://github.com/GyverLibs/StreamPacket/releases/latest/download/StreamPacket.zip)
[![PIO](https://badges.registry.platformio.org/packages/gyverlibs/library/StreamPacket.svg)](https://registry.platformio.org/libraries/gyverlibs/StreamPacket)
[![Foo](https://img.shields.io/badge/Website-AlexGyver.ru-blue.svg?style=flat-square)](https://alexgyver.ru/)
[![Foo](https://img.shields.io/badge/%E2%82%BD%24%E2%82%AC%20%D0%9F%D0%BE%D0%B4%D0%B4%D0%B5%D1%80%D0%B6%D0%B0%D1%82%D1%8C-%D0%B0%D0%B2%D1%82%D0%BE%D1%80%D0%B0-orange.svg?style=flat-square)](https://alexgyver.ru/support_alex/)
[![Foo](https://img.shields.io/badge/README-ENGLISH-blueviolet.svg?style=flat-square)](https://github-com.translate.goog/GyverLibs/StreamPacket?_x_tr_sl=ru&_x_tr_tl=en)  

[![Foo](https://img.shields.io/badge/ПОДПИСАТЬСЯ-НА%20ОБНОВЛЕНИЯ-brightgreen.svg?style=social&logo=telegram&color=blue)](https://t.me/GyverLibs)

# StreamPacket
Simple universal protocol for data transmission via Arduino Stream (Serial, Wire, etc.)
- Integrity control
- 256 package types
- Asynchronous reception
- Possibility of chunked sending

### Compatibility
Compatible with all Arduino platforms (Arduino features are used)

## Contents
- [Use of use](#usage)
- [Versions](#versions)
- [Installation](#install)
- [Bugs and feedback](#feedback)

<a id="usage"></a>

## Use of use
The library is designed to efficiently send and read packets over the Arduino Stream. When a packet is sent, the packet type is assigned the number 8 bits. When parsing, this number will be available in order to cast the data into the desired expected type. For example, send 32 bit number, packet type 1 and string piece, packet type 2:

```cpp
uint32_t val = 12345;
StreamPacket::send(Serial, 1, val);     // Packet type 1 (length autodetermination)

char str[] = "hello";
StreamPacket::send(Serial, 2, str, 5);  // Packet type 2, length 5 (without null symbol)
```

The library will compose and send a package, it has the following structure:

- Title:
  - Starting byte
  - Length (2 bytes)
  - Type (1 byte)
  - CRC (1 byte)
- Data data
  - Data data
  - CRC (1 byte)

When parsing, the library will check the correctness of the data and send it to the processor. In it, you can disassemble data based on the type of packet, and cast them into the desired type. The library also sends packet length, which allows you to receive dynamic data:

```cpp
void loop() {
    StreamPacket::parse(Serial, [](uint8_t type, void* data, size_t len){
        switch (type) {
            case 1:
                // type 1 - 32 bits whole
                Serial.println("type 1");
                Serial.println(*((uint32_t*)data));
                break;
            
            case 2:
                // Type 2 - string (lengths len)
                Serial.println("type 2");
                Serial.write((char*)data, len);
                Serial.println();
                break;
        }
    });
}
```

It is most convenient to use to transfer the structure - see the examples below.

## Description of classes
### StreamPacket
Synchronous sending and receiving

```cpp
// send out
bool StreamPacket::send(Stream& s, Tp type, const Td& data);

// send out
bool StreamPacket::send(Stream& s, Tp type, const void* data, size_t len);

// Collaboration of f(uint8 t type, void* data, size t len)
void StreamPacket::parse(Stream& s, ParseCallback cb);
```

### StreamPacket::Sender
Chunked shipper

```cpp
Sender(Stream& s);

// packet
bool beginPacket(Tp type, size_t len);

// chunk
bool send(const Td& data);

// chunk
bool send(const void* data, size_t len);

// complete
bool endPacket();
```

### StreamPacket::Reader
Asynchronous parser, running on the Stream buffer (packages up to 64 bytes AVR, 128 bytes ESP)

```cpp
// callback type f(uint8 t type, void* data, size t len)
Reader(Stream& s, ParseCallback cb = nullptr);

// callback type f(uint8 t type, void* data, size t len)
void onData(ParseCallback cb);

// ticker
void tick();
```

### StreamPacket::ReaderBuf
Asynchronous parser with its buffer for any size

```cpp
// callback type f(uint8 t type, void* data, size t len)
ReaderBuf(Stream& s, ParseCallback cb = nullptr);

// callback type f(uint8 t type, void* data, size t len)
void onData(ParseCallback cb);

// ticker
void tick();
```

## Examples
### Sending.
```cpp
struct S {
    int a;
    float b;
    char str[5];
};

S s{123, 3.14, "abcd"};
StreamPacket::send(Serial, 1, s);   // packet-type
```

```cpp
char str[] = "hello";
StreamPacket::send(Serial, 2, str, 5);  // package type 2, length 5
```

### Chunked shipping
```cpp
StreamPacket::Sender s(Serial);
s.beginPacket(3, 17);   // Packet type 3, length 17 bytes
s.send("hello ", 6);
s.send("world ", 6);
s.send("12345", 5);
s.endPacket();
```

### Reception.
```cpp
void loop() {
    StreamPacket::parse(Serial, [](uint8_t type, void* data, size_t len){
        switch (type) {
            case 1:
                Serial.println("type 1");
                Serial.write((char*)data, len);
                break;
            
            case 2:
                Serial.println("type 2");
                Serial.write((char*)data, len);
                break;

            case 3: {
                Serial.println("type 3");
                S& s = *((S*)data);
                Serial.println(s.a);
                Serial.println(s.b);
                Serial.println(s.str);
            } break;
        }
        Serial.println();
    });
}
```
```cpp
StreamPacket::Reader p(Serial, [](uint8_t type, void* data, size_t len){
    switch (type) {
        case 1:
            Serial.println("type 1");
            Serial.write((char*)data, len);
            break;
        
        case 2:
            Serial.println("type 2");
            Serial.write((char*)data, len);
            break;

        case 3: {
            Serial.println("type 3");
            S& s = *((S*)data);
            Serial.println(s.a);
            Serial.println(s.b);
            Serial.println(s.str);
        } break;
    }
    Serial.println();
});

void loop() {
    p.tick();
}
```

<a id="versions"></a>

## Versions
- v1.0

<a id="install"></a>
## Installation
- The library can be found under the name **StreamPacket** and installed through the library manager in:
    - Arduino IDE
    - Arduino IDE v2
    - PlatformIO
- [Download the library](https://github.com/GyverLibs/StreamPacket/archive/refs/heads/main.zip).zip archive for manual installation:
    - Unpack and put in *C:\Program Files (x86)\Arduino\libraries* (Windows x64)
    - Unpack and put in *C:\Program Files\Arduino\libraries* (Windows x32)
    - Unpack and put in *Documents/Arduino/libraries/ *
    - (Arduino IDE) Automatic installation from .zip: *Sketch/Connect library/Add .ZIP library...* and specify downloaded archive
- Read more detailed instructions for installing libraries[here](https://alexgyver.ru/arduino-first/#%D0%A3%D1%81%D1%82%D0%B0%D0%BD%D0%BE%D0%B2%D0%BA%D0%B0_%D0%B1%D0%B8%D0%B1%D0%BB%D0%B8%D0%BE%D1%82%D0%B5%D0%BA)
### Update
- I recommend always updating the library: new versions fix errors and bugs, as well as optimize and add new features.
- Through the library manager IDE: find the library as when installing and click "Update"
- Manually: **Delete the folder with the old version** and then put the new one in its place. “Replacement” can not be done: sometimes new versions delete files that will remain when replaced and can lead to errors!

<a id="feedback"></a>

## Bugs and feedback
If you find bugs, create **Issue**, or better write to the mail immediately.[alex@alexgyver.ru](mailto:alex@alexgyver.ru)  
The library is open for revision and your **Pull Requests*!

When reporting bugs or incorrect work of the library, it is necessary to specify:
- Library version
- What is used by the IC
- SDK version (for ESP)
- Arduino IDE version
- Are embedded examples that use features and designs that cause bugs in your code working correctly?
- What code was downloaded, what work was expected from it and how it works in reality
- Ideally, attach the minimum code in which the bug is observed. Not a canvas of a thousand lines, but a minimum code.
