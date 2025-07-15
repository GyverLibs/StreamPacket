This is an automatic translation, may be incorrect in some places. See sources and examples!

# Streampacket
A simple universal data transfer protocol through Arduino Stream (Serial, Wire, etc.
- Control of integrity
- 256 types of packages
- Asynchronous reception
- Oppressing Chunked Sending

## compatibility
Compatible with all arduino platforms (used arduino functions)

## Content
- [use] (#usage)
- [versions] (#varsions)
- [installation] (# Install)
- [bugs and feedback] (#fedback)

<a id = "USAGE"> </A>

## Usage
The library is intended for effective sending and reading packages on Arduino Stream.When sending, the package is assigned the type of package - the number 8 bits.With parsing, this number will be available in order to cast data into the desired type.For example, send 32 bits the number, type of package 1 and a piece of string, type of package 2:

`` `CPP
uint32_t val = 12345;
Streampacket :: Send (Serial, 1, Val);// Type of package 1 (auto -determination of length)

Chard [] = "Hello";
Streampacket :: Send (Serial, 2, Str, 5);// Type of package 2, length 5 (without a zero symbol)
`` `

The library will compose and send a package, it has the following structure:

- Title
- Starting byte
- length (2 bytes)
- Type (1 byte)
- CRC (1 byte)
- Data
- Data
- CRC (1 byte)

During parsing, the library will check the correctness of the data and send them to the handler.In it you can disassemble the data based on the type of package, and cast it in the desired type.The library also sends the length of the package, which allows you to accept dynamic data:

`` `CPP
VOID loop () {
Streampacket :: Parse (serial, [] (uint8_t type, void* data, size_t len) {
switch (type) {
Case 1:
// Type 1 - 32 bit the whole
Serial.println ("type 1");
Serial.println (*((uint32_t*) data));
Break;

Case 2:
// Type 2 - line (LEN lengths)
Serial.println ("type 2");
Serial.write ((char*) Data, Len);
Serial.println ();
Break;
}
});
}
`` `

It is most convenient to use to transfer the structure - see examples below.

## description of classes
### Streampacket
Synchronous sending and reception

`` `CPP
// Send data
Bool Streampacket :: Send (Stream & S, Tp Type, Consta TD & DATA);

// Send data
Bool Streampacket :: Send (Stream & S, Tp Type, Consta Void* Data, Size_t Len);

// Parish the data, collbe of the type f (uint8_t type, void* data, size_t len)
VOID Streampacket :: Parse (Stream & S, Parsecallback CB);
`` `

### Streampacket :: Sender
Chunked SPIRITER

`` `CPP
Sender (Stream & S);

// Start a package
Bool BeginPacket (TP Type, Size_t Len);

// Send a chambers
Bool Send (Consta TD & DATA);

// Send a chambers
Bool Send (Consta* Data, Size_t Len);

// Complete sending
Bool Endpacket ();
`` `

## streampacket :: Reader
Asynchronous Parser, works on the Stream buffer (packages up to 64 bytes AVR, 128 bytes ESP)

`` `CPP
// Collback type F (uint8_t Type, VOID* DATA, SIZE_T LEN)
Reader (Stream & S, Parsecallback CB = NULLPTR);

// Collback type F (uint8_t Type, VOID* DATA, SIZE_T LEN)
VOID ONDATA (PARSECALLBACK CB);

// ticker, call in loop
VOID Tick ();
`` `

## streampacket :: Readerbuf
Asynchronous Parser with his buffer to any size

`` `CPP
// Collback type F (uint8_t Type, VOID* DATA, SIZE_T LEN)
Readerbuf (Stream & S, Parsecallback CB = NULLPTR);

// Collback type F (uint8_t Type, VOID* DATA, SIZE_T LEN)
VOID ONDATA (PARSECALLBACK CB);

// ticker, call in loop
VOID Tick ();
`` `

## Examples
### Sending
`` `CPP
Struct S {
int a;
Float b;
Char str [5];
};

S s {123, 3.14, "abcd"};
Streampacket :: Send (Serial, 1, S);// Type of package 1
`` `

`` `CPP
Chard [] = "Hello";
Streampacket :: Send (Serial, 2, Str, 5);// Type of package 2, length 5
`` `

### Chunked Sending
`` `CPP
Streampacket :: Sender S (Serial);
S.Beginpacket (3, 17);// Type of package 3, length 17 bytes
S.Send ("HELLO", 6);
S.Send ("World", 6);
S.Send ("12345", 5);
S.endpacket ();
`` `

### Reception
`` `CPP
VOID loop () {
Streampacket :: Parse (serial, [] (uint8_t type, void* data, size_t len) {
switch (type) {
Case 1:
Serial.println ("type 1");
Serial.write ((char*) Data, Len);
Break;

Case 2:
Serial.println ("type 2");
Serial.write ((char*) Data, Len);
Break;

Case 3: {
Serial.println ("type 3");
S & s = *((s *) data);
Serial.println (S.A);
Serial.println (S.B);
Serial.println (S.str);
} Break;
}
Serial.println ();
});
}
`` `
`` `CPP
Streampacket :: Reader p (serial, [] (uint8_t type, void* data, size_t len) {
switch (type) {
Case 1:
Serial.println ("type 1");
Serial.write ((char*) Data, Len);
Break;

Case 2:
Serial.println ("type 2");
Serial.write ((char*) Data, Len);
Break;

Case 3: {
Serial.println ("type 3");
S & s = *((s *) data);
Serial.println (S.A);
Serial.println (S.B);
Serial.println (S.str);
} Break;
}
Serial.println ();
});

VOID loop () {
P.Tick ();
}
`` `

<a ID = "Versions"> </a>

## versions
- V1.0

<a id = "Install"> </a>
## Installation
- The library can be found by the name ** Streampacket ** and installed through the library manager in:
- Arduino ide
- Arduino ide v2
- Platformio
- [download the library] (https://github.com/gyverlibs/streampacket/archive/refs/heads/main.zip) .Zip archive for manual installation:
- unpack and put in * C: \ Program Files (X86) \ Arduino \ Libraries * (Windows X64)
- unpack and put in * C: \ Program Files \ Arduino \ Libraries * (Windows X32)
- unpack and put in *documents/arduino/libraries/ *
- (Arduino id) Automatic installation from. Zip: * sketch/connect the library/add .Zip library ... * and specify downloaded archive
- Read more detailed instructions for installing libraries[here] (https://alexgyver.ru/arduino-first/#%D0%A3%D1%81%D1%82%D0%B0%D0%BD%D0%BE%D0%B2%D0%BA%D0%B0_%D0%B1%D0%B8%D0%B1%D0%B8%D0%BE%D1%82%D0%B5%D0%BA)
### Update
- I recommend always updating the library: errors and bugs are corrected in the new versions, as well as optimization and new features are added
- through the IDE library manager: find the library how to install and click "update"
- Manually: ** remove the folder with the old version **, and then put a new one in its place.“Replacement” cannot be done: sometimes in new versions, files that remain when replacing are deleted and can lead to errors!

<a id = "Feedback"> </a>

## bugs and feedback
Create ** Issue ** when you find the bugs, and better immediately write to the mail [alex@alexgyver.ru] (mailto: alex@alexgyver.ru)
The library is open for refinement and your ** pull Request ** 'ow!

When reporting about bugs or incorrect work of the library, it is necessary to indicate:
- The version of the library
- What is MK used
- SDK version (for ESP)
- version of Arduino ide
- whether the built -in prImers that use the functions and designs leading to a bug in your code
- what code has been loaded, what work was expected from it and how it works in reality
- Ideally, attach the minimum code in which the bug is observed.Not a canvas of a thousand lines, but a minimum code