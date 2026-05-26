# Shifter / Button Box SimRacing (ESP32-S3)

Niestandardowy Shifter / Button Box do SimRacingu używający układu ESP32-S3. Wykrywany jako standardowy joystick USB z przyciskami.

## Funkcje
- **8 Przycisków (Shifter):** Obsługa przycisków cyfrowych (np. do skrzyni biegów, kierownicy lub panelu akcesoriów).
- **Prosty Tester:** Dedykowana aplikacja PC (ciemny motyw) pozwala na błyskawiczne sprawdzenie działania wciśniętych przycisków.

## Wymagany Sprzęt
- ESP32-S3 DevKitC-1
- Przełączniki / Przyciski chwilowe (max 8)
- Kabel USB

## Podłączenie (Wiring)

### Przyciski / Shifter (Cyfrowe)
Piny GPIO: **4, 5, 6, 7, 8, 9, 10, 11**

**Podłączenie:**
- Jeden styk przycisku do **GND**.
- Drugi styk przycisku do wybranego pinu **GPIO**.
*(Wewnętrzne rezystory Pull-Up są włączone programowo).*

## Instalacja (Łatwy Sposób)

### 1. Wgrywanie Firmware
1.  Pobierz najnowszą wersję (zawierającą `firmware.bin` w folderze `bin`).
2.  Zainstaluj [Python](https://www.python.org/downloads/).
3.  Podłącz ESP32-S3 przez USB.
    *   *Jeśli nie jest wykrywane, przytrzymaj przycisk BOOT podczas podłączania.*
4.  Uruchom `flash_firmware.bat` (Windows) lub `./flash_firmware.sh` (Mac/Linux).
5.  Postępuj zgodnie z instrukcjami, aby wpisać port COM (np. `COM3` lub `/dev/ttyACM0`).

### 2. Testowanie Połączeń
1.  Uruchom `run_configurator.bat` (Windows) lub:
    ```bash
    pythonw pedals-configurator/configurator.pyw
    ```
2.  Aplikacja uruchomi się bez okna konsoli. Wybierz port COM i kliknij **Połącz**.
3.  Wciskaj swoje przyciski. Na ekranie powinny natychmiast zapalać się na zielono odpowiadające im numery, potwierdzając poprawne podłączenie do GPIO.

## Zaawansowana Konfiguracja (Dla Programistów)
Jeśli chcesz modyfikować kod (np. zwiększyć ilość przycisków):
1.  Zainstaluj [PlatformIO](https://platformio.org/).
2.  Otwórz folder `pedals-firmware`.
3.  Edytuj `src/main.cpp` (zmień deskryptor HID, dodaj/zmień piny).
4.  Skompiluj i wgraj (Build and Upload) bezpośrednio z PlatformIO.
