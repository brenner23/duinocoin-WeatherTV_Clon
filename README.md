# WeatherTV-Clon – Duino-Coin Miner (ESP8266, PlatformIO)

**Stand: 16.09.2026**  
**Hardware-Basis:** ESP8266-Modul ESP-12F (4 MB Flash) + ST7789 240×240

Duino-Coin-Miner für die kleinen ESP8266-Wetteranzeigen (GeekMagic „SmallTV“ in Gelb, schwarze Nachbau-Boards und HelloCube). Die Version kombiniert den Miner mit einer kompakten 240×240-Anzeige, lokalem Web-Dashboard, OTA-Updates und einer Crash-Blackbox.

Abgeleitet von der ESP32-Version für das NM-TV-154 ([duinocoin-NM-TV-154-Miner](https://github.com/brenner23/duinocoin-NM-TV-154-Miner)) und auf den ESP8266 zurückgebaut.

## Features

- Duino-Coin Mining (DSHA1) auf dem ESP8266 – **ein Kern**, CPU-Takt 160 MHz
- ST7789 240×240 Display mit kompakter Miner-Übersicht
- Anzeige von Hashrate, Difficulty, Shares und JOBS
- **JOBS-Zähler** als Aktivitätsanzeige (0–999999, danach Neustart bei 0)
- Automatisch kleinere Schrift bei langen JOBS-Werten
- WLAN-Status / Signalstärke auf dem Display
- Lokales Web-Dashboard mit Miner- und Diagnosewerten
- OTA-Update über das Netzwerk (passwortgeschützt)
- **Crash-/Boot-Blackbox** mit Reset-Ursache und Checkpoints
- Speicherung von bis zu 10 echten Abstürzen im **EEPROM** (Hardware-Watchdog, Software-Watchdog, Exception)
- Normales Einschalten, Reset-Taster und Web-Neustart zählen nicht als Absturz
- Crashlog im Web-Dashboard einsehbar und löschbar
- DSHA1-Warmup gegen Out-of-Bounds-Zugriff abgesichert
- Explizite Einbindung verwendeter Display-Fonts

## Anzeige

Die Hauptanzeige zeigt die wichtigsten Minerwerte direkt auf dem 240×240-Display. Das frühere Ping-/Best-Diff-Feld wird als **JOBS** verwendet. Der Zähler beginnt nach einem Neustart bei 0 und zeigt, dass der Miner laufend neue Jobs empfängt und verarbeitet.

`BEST DIFF` wurde bewusst nicht übernommen: Der DUCO-Miningablauf sucht nach einem exakten SHA-1-Zielhash. Eine Bitcoin-artige „Best Difficulty“ aus beliebigen getesteten Nonces wäre daher keine aussagekräftige Kennzahl.

## Boards (PlatformIO-Umgebungen)

| Umgebung | Board | Hinweis |
|---|---|---|
| `Yellow` / `Yellow_ota` | ESP-12E/F, 4 MB | gelbes Original-Board |
| `Black4M` / `Black_ota4M` | ESP-12E/F, 4 MB | schwarzes Bastel-Board (Standard) |
| `Black` / `Black_ota` | ESP-01, 1 MB | ältere Variante für Boards mit nur 1 MB Flash (64 KB SPIFFS) |
| `Cube_USB` / `Cube_OTA` | ESP-12E/F, 4 MB | HelloCube – setzt das Flag `CUBE_MIRROR`; die Spiegelung selbst ist in dieser Version noch nicht im Code umgesetzt |

Die `*_ota`-Umgebungen flashen über das Netzwerk – IP-Adresse (`upload_port`) und OTA-Passwort (`--auth`) in `platformio.ini` an das eigene Gerät anpassen.

## Konfiguration

Miner-, WLAN- und OTA-Einstellungen liegen in `include/Settings.h`. Vor dem Bauen die eigenen Werte eintragen (Duino-Coin-Benutzername, Mining-Key, WLAN) und ein eigenes OTA-Passwort vergeben – in `Settings.h` und passend dazu in `platformio.ini`.

## PlatformIO

Das Projekt ist für PlatformIO aufgebaut (`platform = espressif8266`, Arduino-Framework). Benötigte Bibliotheken: Adafruit GFX, Adafruit ST7735/ST7789, ArduinoJson – PlatformIO lädt sie automatisch.

## Versionsstand

**4.3** (siehe `SOFTWARE_VERSION` in `include/Settings.h`)

## Hinweis für weitere Geräte

Hardwareabhängige Teile wie Display-Pins, SPI und Spiegelung müssen beim Übertragen auf andere Boards angepasst werden (`include/pins.h`). Die Pinbelegung sollte daher nicht blind auf andere Boards übernommen werden.
