[![arduino-library-badge](https://www.ardu-badge.com/badge/ServoSmooth.svg?)](https://www.ardu-badge.com/ServoSmooth)
[![Foo](https://img.shields.io/badge/Website-AlexGyver.ru-blue.svg?style=flat-square)](https://alexgyver.ru/)
[![Foo](https://img.shields.io/badge/%E2%82%BD$%E2%82%AC%20%D0%9D%D0%B0%20%D0%BF%D0%B8%D0%B2%D0%BE-%D1%81%20%D1%80%D1%8B%D0%B1%D0%BA%D0%BE%D0%B9-orange.svg?style=flat-square)](https://alexgyver.ru/support_alex/)
[![Foo](https://img.shields.io/badge/README-ENGLISH-blueviolet.svg?style=flat-square)](https://github-com.translate.goog/GyverLibs/ServoSmooth?_x_tr_sl=ru&_x_tr_tl=en)  

[![Foo](https://img.shields.io/badge/ПОДПИСАТЬСЯ-НА%20ОБНОВЛЕНИЯ-brightgreen.svg?style=social&logo=telegram&color=blue)](https://t.me/GyverLibs)

# minimum DRO
Проект контроллера УЦИ (DRO) с минимально необходимым функционалом для токарного станка WM210V

!!! В процессе разработки !!!

Shahe scale - inverter level converter K561LN2 - esp32 
 + level converter for 5V TM1637 6-digits x3
 + keypad 4x4 PCF8574 - X0 Z0 ABS/INC Enter
 + switch Radius/Diametr
 + switch RPM/Speed
 + 22 mm power button
 + tachometr input
 + probe input + Buzzer = calibrator

3D print case with magnets/GoPro mount
 - print table work mode
 - power in 7-24V

future:
 BT SPP for Android TouchDRO
 WiFi - MGX3D_EspDRO

### Совместимость
Совместима со всеми Arduino платформами (используются Arduino-функции)

### Документация

## Содержание
- [Используемые модули](#init)
- [Схема электроники](#schemes)
- [Корпус](#case)
- [Прошивка](#install)
- [Использование](#usage)
- [Рабочий режим](#workmode)
- [Режим настройки](#setupmode)
- [Версии](#versions)
- [Баги и обратная связь](#feedback)

<a id="init"></a>
## Используемые модули

<a id="schemes"></a>
## Схема электроники

<a id="case"></a>
## Корпус

<a id="install"></a>
## Прошивка
- [Скачать библиотеку](https://github.com/marshalab/minimum-DRO/archive/refs/heads/main.zip) .zip архивом для ручной установки:
    - Распаковать и положить в *C:\Program Files (x86)\Arduino\libraries* (Windows x64)
    - Распаковать и положить в *C:\Program Files\Arduino\libraries* (Windows x32)
    - Распаковать и положить в *Документы/Arduino/libraries/*
    - (Arduino IDE) автоматическая установка из .zip: *Скетч/Подключить библиотеку/Добавить .ZIP библиотеку…* и указать скачанный архив


<a id="#usage"></a>
## Использование

<a id="#workmode"></a>
### Рабочий режим

<a id="#setupmode"></a>
### Режим настройки

<a id="versions"></a>
## Версии
- v0.1a - собрал рабочее устройство и выложил на guthub для дальнейшей доработки через git

<a id="feedback"></a>
## Баги и обратная связь
При нахождении багов создавайте **Issue**, а лучше сразу пишите на почту [marshallab@mail.ru](mailto:marshallab@mail.ru)  
Проект открыт для доработок и ваших **Pull Request**'ов!
