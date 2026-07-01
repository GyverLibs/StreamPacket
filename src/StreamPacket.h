#pragma once
#include <Arduino.h>

#ifndef SP_START
#define SP_START '$'
#endif
#ifndef SP_TOUT
#define SP_TOUT 10
#endif
#ifndef SP_READER_SIZE
#ifdef __AVR__
#define SP_READER_SIZE 64
#else
#define SP_READER_SIZE 128
#endif
#endif

class StreamPacket {
    typedef void (*ParseCallback)(uint8_t type, void* data, size_t len);

    struct Packet {
        uint16_t length;
        uint8_t type;
        uint8_t crc;
    };

   public:
    class Sender;
    class Reader;
    class ReaderBuf;

    // ================= STATIC =================

    // отправить данные
    template <typename Tp, typename Td>
    static bool send(Print& s, Tp type, const Td& data) {
        return send(s, type, &data, sizeof(Td));
    }

    // отправить данные
    template <typename Tp>
    static bool send(Print& s, Tp type, const void* data, size_t len) {
        return _beginSend(s, (uint8_t)type, len) &&
               s.write((const uint8_t*)data, len) == len &&
               s.write(_crc8(data, len)) == 1;
    }

    // парсить данные, коллбэк вида f(uint8_t type, void* data, size_t len)
    static void parse(Stream& s, ParseCallback cb) {
        Packet p;

        if ((size_t)s.available() < _HEADER_SIZE + 2 ||
            s.read() != SP_START ||
            !_readHeader(s, p) ||
            p.length == UINT16_MAX) return;

        size_t len = (size_t)p.length + 1;
        uint8_t* data = new uint8_t[len];
        if (!data) return;

        if (s.readBytes(data, len) == len && !_crc8(data, len) && cb) {
            cb(p.type, data, p.length);
        }

        delete[] data;
    }

    // ================= PRIVATE =================
   private:
    static const uint8_t _HEADER_SIZE = 4;

    static uint8_t _crc8(const void* data, size_t len, uint8_t crc = 0) {
        const uint8_t* p = (const uint8_t*)data;
#ifdef __AVR__
        while (len--) {
            uint8_t data = *p++;
            uint8_t counter;
            uint8_t buffer;
            asm volatile(
                "EOR %[crc_out], %[data_in] \n\t"
                "LDI %[counter], 8          \n\t"
                "LDI %[buffer], 0x8C        \n\t"
                "_loop_start_%=:            \n\t"
                "LSR %[crc_out]             \n\t"
                "BRCC _loop_end_%=          \n\t"
                "EOR %[crc_out], %[buffer]  \n\t"
                "_loop_end_%=:              \n\t"
                "DEC %[counter]             \n\t"
                "BRNE _loop_start_%="
                : [crc_out] "=r"(crc), [counter] "=d"(counter), [buffer] "=d"(buffer)
                : [crc_in] "0"(crc), [data_in] "r"(data));
        }
#else
        while (len--) {
            uint8_t b = *p++, j = 8;
            while (j--) {
                crc = ((crc ^ b) & 1) ? (crc >> 1) ^ 0x8C : (crc >> 1);
                b >>= 1;
            }
        }
#endif
        return crc;
    }

    // начать отправку
    static bool _beginSend(Print& s, uint8_t type, size_t len) {
        if (len >= UINT16_MAX) return false;

        uint8_t header[_HEADER_SIZE] = {
            (uint8_t)len,
            (uint8_t)(len >> 8),
            type,
            0,
        };
        header[3] = _crc8(header, 3);
        return s.write((uint8_t)SP_START) == 1 &&
               s.write(header, sizeof(header)) == sizeof(header);
    }

    static bool _readHeader(Stream& s, Packet& p) {
        uint8_t header[_HEADER_SIZE];
        if (s.readBytes(header, sizeof(header)) != sizeof(header) ||
            _crc8(header, sizeof(header))) return false;

        p.length = (uint16_t)header[0] | ((uint16_t)header[1] << 8);
        p.type = header[2];
        p.crc = header[3];
        return true;
    }
};

// ================= Sender =================

// chunked отправщик
class StreamPacket::Sender {
   public:
    Sender(Print& s) : _s(s) {}

    // начать пакет
    template <typename Tp>
    bool beginPacket(Tp type, size_t len) {
        if (_active) return false;
        _crc = 0;
        _left = len;
        _active = StreamPacket::_beginSend(_s, (uint8_t)type, len);
        return _active;
    }

    // отправить чанк
    template <typename Td>
    bool send(const Td& data) {
        return send(&data, sizeof(Td));
    }

    // отправить чанк
    bool send(const void* data, size_t len) {
        if (!_active || len > _left) return false;

        size_t written = _s.write((const uint8_t*)data, len);
        _crc = StreamPacket::_crc8(data, written, _crc);
        _left -= written;
        if (written != len) _ok = false;
        return written == len;
    }

    // завершить отправку
    bool endPacket() {
        bool valid = _active && _ok && !_left;
        bool sent = valid && _s.write(_crc) == 1;
        _active = false;
        _ok = true;
        return sent;
    }

   private:
    Print& _s;
    uint8_t _crc = 0;
    size_t _left = 0;
    bool _active = false;
    bool _ok = true;
};

// ================= READER BUF =================

// асинхронный парсер со своим буфером
class StreamPacket::ReaderBuf {
   public:
    ReaderBuf(Stream& s, ParseCallback cb = nullptr) : _s(s), _cb(cb) {}
    ~ReaderBuf() {
        delete[] _data;
    }

    ReaderBuf(const ReaderBuf&) = delete;
    ReaderBuf& operator=(const ReaderBuf&) = delete;

    // коллбэк вида f(uint8_t type, void* data, size_t len)
    void onData(ParseCallback cb) {
        _cb = cb;
    }

    // тикер, вызывать в loop
    void tick() {
        if (_data) {
            if (_s.available()) {
                _tmr = millis();
                size_t left = (size_t)_p.length + 1 - _idx;
                size_t rlen = min(left, (size_t)_s.available());

                if (_s.readBytes(_data + _idx, rlen) != rlen) {
                    _reset();
                    return;
                }

                _p.crc = _crc8(_data + _idx, rlen, _p.crc);
                _idx += rlen;

                if (_idx == (size_t)_p.length + 1) {
                    if (!_p.crc && _cb) _cb(_p.type, _data, _p.length);
                    _reset();
                }

            } else {
                if (millis() - _tmr >= SP_TOUT) {
                    _reset();
                }
            }

        } else {
            if ((size_t)_s.available() < _HEADER_SIZE + 1 ||
                _s.read() != SP_START ||
                !_readHeader(_s, _p) ||
                _p.length == UINT16_MAX) return;

            _data = new uint8_t[(size_t)_p.length + 1];
            if (!_data) return;

            _idx = 0;
            _p.crc = 0;
            _tmr = millis();
        }
    }

   private:
    void _reset() {
        delete[] _data;
        _data = nullptr;
    }

    Stream& _s;
    uint8_t* _data = nullptr;
    ParseCallback _cb = nullptr;
    Packet _p;
    uint32_t _tmr = 0;
    size_t _idx = 0;
};

// ================= READER =================

// асинхронный парсер
class StreamPacket::Reader {
   public:
    Reader(Stream& s, ParseCallback cb = nullptr) : _s(s), _cb(cb) {}

    // коллбэк вида f(uint8_t type, void* data, size_t len)
    void onData(ParseCallback cb) {
        _cb = cb;
    }

    // тикер, вызывать в loop
    void tick() {
        if (_reading) {
            if ((uint16_t)_s.available() >= _p.length) {
                uint8_t buf[SP_READER_SIZE + 1];
                if (_s.readBytes(buf, _p.length) == _p.length && _cb && !_crc8(buf, _p.length)) {
                    _cb(_p.type, buf, _p.length - 1);
                }
                _reading = false;
            } else if (_pAv != (uint16_t)_s.available()) {
                _pAv = _s.available();
                _tmr = millis();
            } else if (uint16_t(uint16_t(millis()) - _tmr) >= SP_TOUT) {
                _reading = false;
            }
        } else {
            if ((size_t)_s.available() < _HEADER_SIZE + 1 ||
                _s.read() != SP_START ||
                !_readHeader(_s, _p) ||
                _p.length > SP_READER_SIZE) {
                return;
            }
            ++_p.length;
            _reading = true;
            _tmr = millis();
            _pAv = 0;
        }
    }

   private:
    Stream& _s;
    ParseCallback _cb = nullptr;
    Packet _p;
    uint16_t _pAv = 0;
    uint16_t _tmr = 0;
    bool _reading = false;
};
