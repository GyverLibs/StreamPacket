[![latest](https://img.shields.io/github/v/release/GyverLibs/StreamPacket.svg?color=brightgreen)](https://github.com/GyverLibs/StreamPacket/releases/latest/download/StreamPacket.zip)
[![PIO](https://badges.registry.platformio.org/packages/gyverlibs/library/StreamPacket.svg)](https://registry.platformio.org/libraries/gyverlibs/StreamPacket)
[![Foo](https://img.shields.io/badge/Website-AlexGyver.ru-blue.svg?style=flat-square)](https://alexgyver.ru/)
[![Foo](https://img.shields.io/badge/%E2%82%BD%24%E2%82%AC%20%D0%9F%D0%BE%D0%B4%D0%B4%D0%B5%D1%80%D0%B6%D0%B0%D1%82%D1%8C-%D0%B0%D0%B2%D1%82%D0%BE%D1%80%D0%B0-orange.svg?style=flat-square)](https://alexgyver.ru/support_alex/)
[![Foo](https://img.shields.io/badge/README-ENGLISH-blueviolet.svg?style=flat-square)](https://github-com.translate.goog/GyverLibs/StreamPacket?_x_tr_sl=ru&_x_tr_tl=en)  

[![Foo](https://img.shields.io/badge/ПОДПИСАТЬСЯ-НА%20ОБНОВЛЕНИЯ-brightgreen.svg?style=social&logo=telegram&color=blue)](https://t.me/GyverLibs)

# StreamPacket
Простой универсальный протокол для передачи данных через Arduino Stream (Serial, Wire, итд)
- Контроль целостности
- 256 типов пакетов
- Асинхронный приём
- Возможность chunked отправки

### Совместимость
Совместима со всеми Arduino платформами (используются Arduino-функции)

## Содержание
- [Использование](#usage)
- [Версии](#versions)
- [Установка](#install)
- [Баги и обратная связь](#feedback)

<a id="usage"></a>

## Использование
Библиотека предназначена для эффективной отправки и чтения пакетов по Arduino Stream. При отправке пакету присваивается тип пакета - число 8 бит. При парсинге это число будет доступно для того, чтобы кастовать данные в нужный ожидаемый тип. Например отправим 32 бит число, тип пакета 1 и кусок строки, тип пакета 2:

```cpp
uint32_t val = 12345;
StreamPacket::send(Serial, 1, val);     // тип пакета 1 (автоопределение длины)

char str[] = "hello";
StreamPacket::send(Serial, 2, str, 5);  // тип пакета 2, длина 5 (без нулевого символа)
```

Библиотека составит и отправит пакет, он имеет следующую структуру:

- Заголовок
  - Стартовый байт
  - Длина (2 байта)
  - Тип (1 байт)
  - CRC (1 байт)
- Данные
  - Данные
  - CRC (1 байт)

При парсинге библиотека проверит корректность данных и отправит их в обработчик. В нём можно разобрать данные, исходя из типа пакета, и кастовать их в нужный тип. Библиотека также отправляет длину пакета, что позволяет принимать динамические данные:

```cpp
void loop() {
    StreamPacket::parse(Serial, [](uint8_t type, void* data, size_t len){
        switch (type) {
            case 1:
                // тип 1 - 32 бит целое
                Serial.println("type 1");
                Serial.println(*((uint32_t*)data));
                break;
            
            case 2:
                // тип 2 - строка (длины len)
                Serial.println("type 2");
                Serial.write((char*)data, len);
                Serial.println();
                break;
        }
    });
}
```

Удобнее всего использовать для передачи структуры - см. примеры ниже.

## Описание классов
### StreamPacket
Синхронная отправка и приём

```cpp
// отправить данные
bool StreamPacket::send(Stream& s, Tp type, const Td& data);

// отправить данные
bool StreamPacket::send(Stream& s, Tp type, const void* data, size_t len);

// парсить данные, коллбэк вида f(uint8_t type, void* data, size_t len)
void StreamPacket::parse(Stream& s, ParseCallback cb);
```

### StreamPacket::Sender
Chunked отправщик

```cpp
Sender(Stream& s);

// начать пакет
bool beginPacket(Tp type, size_t len);

// отправить чанк
bool send(const Td& data);

// отправить чанк
bool send(const void* data, size_t len);

// завершить отправку
bool endPacket();
```

### StreamPacket::Reader
Асинхронный парсер, работает на буфере Stream (пакеты до 64 байт AVR, 128 байт ESP)

```cpp
// коллбэк вида f(uint8_t type, void* data, size_t len)
Reader(Stream& s, ParseCallback cb = nullptr);

// коллбэк вида f(uint8_t type, void* data, size_t len)
void onData(ParseCallback cb);

// тикер, вызывать в loop
void tick();
```

### StreamPacket::ReaderBuf
Асинхронный парсер со своим буфером на любой размер

```cpp
// коллбэк вида f(uint8_t type, void* data, size_t len)
ReaderBuf(Stream& s, ParseCallback cb = nullptr);

// коллбэк вида f(uint8_t type, void* data, size_t len)
void onData(ParseCallback cb);

// тикер, вызывать в loop
void tick();
```

## Примеры
### Отправка
```cpp
struct S {
    int a;
    float b;
    char str[5];
};

S s{123, 3.14, "abcd"};
StreamPacket::send(Serial, 1, s);   // тип пакета 1
```

```cpp
char str[] = "hello";
StreamPacket::send(Serial, 2, str, 5);  // тип пакета 2, длина 5
```

### Chunked отправка
```cpp
StreamPacket::Sender s(Serial);
s.beginPacket(3, 17);   // тип пакета 3, длина 17 байт
s.send("hello ", 6);
s.send("world ", 6);
s.send("12345", 5);
s.endPacket();
```

### Приём
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

## Версии
- v1.0

<a id="install"></a>
## Установка
- Библиотеку можно найти по названию **StreamPacket** и установить через менеджер библиотек в:
    - Arduino IDE
    - Arduino IDE v2
    - PlatformIO
- [Скачать библиотеку](https://github.com/GyverLibs/StreamPacket/archive/refs/heads/main.zip) .zip архивом для ручной установки:
    - Распаковать и положить в *C:\Program Files (x86)\Arduino\libraries* (Windows x64)
    - Распаковать и положить в *C:\Program Files\Arduino\libraries* (Windows x32)
    - Распаковать и положить в *Документы/Arduino/libraries/*
    - (Arduino IDE) автоматическая установка из .zip: *Скетч/Подключить библиотеку/Добавить .ZIP библиотеку…* и указать скачанный архив
- Читай более подробную инструкцию по установке библиотек [здесь](https://alexgyver.ru/arduino-first/#%D0%A3%D1%81%D1%82%D0%B0%D0%BD%D0%BE%D0%B2%D0%BA%D0%B0_%D0%B1%D0%B8%D0%B1%D0%BB%D0%B8%D0%BE%D1%82%D0%B5%D0%BA)
### Обновление
- Рекомендую всегда обновлять библиотеку: в новых версиях исправляются ошибки и баги, а также проводится оптимизация и добавляются новые фичи
- Через менеджер библиотек IDE: найти библиотеку как при установке и нажать "Обновить"
- Вручную: **удалить папку со старой версией**, а затем положить на её место новую. "Замену" делать нельзя: иногда в новых версиях удаляются файлы, которые останутся при замене и могут привести к ошибкам!

<a id="feedback"></a>

## Баги и обратная связь
При нахождении багов создавайте **Issue**, а лучше сразу пишите на почту [alex@alexgyver.ru](mailto:alex@alexgyver.ru)  
Библиотека открыта для доработки и ваших **Pull Request**'ов!

При сообщении о багах или некорректной работе библиотеки нужно обязательно указывать:
- Версия библиотеки
- Какой используется МК
- Версия SDK (для ESP)
- Версия Arduino IDE
- Корректно ли работают ли встроенные примеры, в которых используются функции и конструкции, приводящие к багу в вашем коде
- Какой код загружался, какая работа от него ожидалась и как он работает в реальности
- В идеале приложить минимальный код, в котором наблюдается баг. Не полотно из тысячи строк, а минимальный код