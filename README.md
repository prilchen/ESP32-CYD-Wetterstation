# ESP32 CYD Wetterstation mit LVGL 9.5 und EEZ Studio

Wetterstation für das ESP32-3248S035R (Cheap Yellow Display / CYD), entwickelt mit EEZ Studio und LVGL 9.5.

> Status: vorbereitet, noch nicht veröffentlicht.

## Funktionen

- Temperatur-Tacho mit LVGL Scale
- Temperatur, Luftfeuchtigkeit und Regenwahrscheinlichkeit
- Windgeschwindigkeit und Windrichtung
- Datum und Uhrzeit aus Open-Meteo
- Aktualisierung alle 15 Minuten
- drei Abrufversuche und Statusanzeige
- Deep Sleep nach 60 Sekunden
- Aufwachen über Touch IRQ an GPIO 36

## Voraussetzungen

- ESP32-3248S035R
- Arduino IDE mit ESP32-Unterstützung
- LVGL 9.5
- TFT_eSPI
- ArduinoJson
- vollständiger EEZ-Studio-Export im Ordner `ui/`

## Einrichtung

1. `secrets.example.h` nach `secrets.h` kopieren.
2. WLAN-Name und Passwort in `secrets.h` eintragen.
3. Die EEZ-Studio-Exportdateien in `ui/` ablegen.
4. Sicherstellen, dass `ui.h` über den Include-Pfad erreichbar ist.
5. Die für das konkrete CYD passende TFT_eSPI-Konfiguration verwenden.
6. In der Arduino IDE `ESP32 Dev Module` und ein ausreichend großes Partitionsschema wählen.
7. Sketch kompilieren und aufspielen.

## Benötigte UI-Objekte

```text
objects.scale_main
objects.scale_needle
objects.label_temp
objects.label_hum
objects.label_wind
objects.label_rain
objects.label_time
objects.label_status
```

## Standort anpassen

Die Open-Meteo-URL im Sketch enthält Koordinaten für Frechen/Köln. Für einen anderen Standort `latitude`, `longitude` und gegebenenfalls `timezone` anpassen.

## Sicherheit

`secrets.h` ist in `.gitignore` eingetragen. WLAN-Zugangsdaten gehören nicht ins Repository.

## Bekannte Voraussetzung

Das Paket enthält keinen erfundenen EEZ-Studio-Export. Ohne die echten Dateien `ui.h`, `ui.c` und Assets ist der Sketch noch nicht vollständig kompilierbar.

## Artikel

Projektbegleitung: [ESP32 + CYD Teil 3 auf Prilchen Labs](https://prilchen.de/esp32-cyd-teil-3-komplette-lvgl-gui-erstellung-mit-eez-studio/)

## Lizenz

Noch nicht festgelegt. Siehe `LICENSE-NOT-SET.txt`.

© Prilchen LABS 2026
