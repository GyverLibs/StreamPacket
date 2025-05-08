#include <Arduino.h>

#define SP_START '$'
#define SP_TOUT 10

class StreamPacket {
    typedef void (*ParseCallback)(uint8_t type, void* data, size_t len);

    struct Packet {
        uint16_t length;
        uint8_t type;
        uint8_t crc;
    };

   public:
    // ================= Sender =================

    // chunked отправщик
    class Sender {
       public:
        Sender(Print& s) : _s(s) {}

        // начать пакет
        template <typename Tp>
        bool beginPacket(Tp type, size_t len) {
            return StreamPacket::_beginSend(_s, type, len);
        }

        // отправить чанк
        template <typename Td>
        bool send(const Td& data) {
            return send(&data, sizeof(Td));
        }

        // отправить чанк
        bool send(const void* data, size_t len) {
            _crc = StreamPacket::_crc8(data, len, _crc);
            return _s.write((uint8_t*)data, len) == len;
        }

        // завершить отправку
        bool endPacket() {
            return _s.write(_crc);
        }

       private:
       Print& _s;
        uint8_t _crc = 0;
    };

    // ================= READER =================

    // асинхронный парсер
    class Reader {
       public:
        Reader(Stream& s, ParseCallback cb = nullptr) : _s(s), _cb(cb) {}

        // коллбэк вида f(uint8_t type, void* data, size_t len)
        void onData(ParseCallback cb) {
            _cb = cb;
        }

        // тикер, вызывать в loop
        void tick() {
            if (_data) {
                if (_s.available()) {
                    _tmr = millis();
                    size_t rlen = min(int(_p.length + 1 - _idx), _s.available());

                    if (_s.readBytes(_data + _idx, rlen) != rlen) {
                        delete _data;
                        _data = nullptr;
                        return;
                    }

                    _p.crc = _crc8(_data + _idx, rlen, _p.crc);
                    _idx += rlen;

                    if (_idx == _p.length + 1) {
                        if (!_p.crc) _cb(_p.type, _data, _p.length);
                        delete _data;
                        _data = nullptr;
                    }

                } else {
                    if (millis() - _tmr >= SP_TOUT) {
                        delete _data;
                        _data = nullptr;
                    }
                }

            } else {
                if ((size_t)_s.available() < sizeof(_p) + 1 ||
                    _s.read() != SP_START ||
                    _s.readBytes((uint8_t*)&_p, sizeof(_p)) != sizeof(_p) ||
                    _crc8(&_p, sizeof(_p))) return;

                _data = new uint8_t[_p.length + 1];
                if (!_data) return;

                _idx = 0;
                _p.crc = 0;
                _tmr = millis();
            }
        }

       private:
        Stream& _s;
        uint8_t* _data = nullptr;
        ParseCallback _cb = nullptr;
        Packet _p;
        uint32_t _tmr = 0;
        uint16_t _idx = 0;
    };

    // ================= STATIC =================

    // отправить данные
    template <typename Tp, typename Td>
    static bool send(Print& s, Tp type, const Td& data) {
        return send(s, type, &data, sizeof(Td));
    }

    // отправить данные
    template <typename Tp>
    static bool send(Print& s, Tp type, const void* data, size_t len) {
        return _beginSend(s, type, len) &&
               s.write((uint8_t*)data, len) == len &&
               s.write(_crc8(data, len)) == 1;
    }

    // парсить данные, коллбэк вида f(uint8_t type, void* data, size_t len)
    static void parse(Stream& s, ParseCallback cb) {
        Packet p;

        if ((size_t)s.available() < sizeof(p) + 2 ||
            s.read() != SP_START ||
            s.readBytes((uint8_t*)&p, sizeof(p)) != sizeof(p) ||
            _crc8(&p, sizeof(p))) return;

        uint16_t len = p.length + 1;
        uint8_t* data = new uint8_t[len];
        if (!data) return;

        if (s.readBytes(data, len) == len && !_crc8(data, len)) {
            cb(p.type, data, p.length);
        }

        delete[] data;
    }

    // ================= PRIVATE =================
   private:
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
    template <typename Tp>
    static bool _beginSend(Print& s, Tp type, size_t len) {
        Packet p{len, (uint8_t)type};
        p.crc = _crc8(&p, sizeof(p) - 1);
        return s.write((uint8_t)SP_START) == 1 && s.write((uint8_t*)&p, sizeof(p)) == sizeof(p);
    }
};