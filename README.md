# Pedały SimRacing (ESP32-S3)

Niestandardowe pedały SimRacing używające układu ESP32-S3. Wykrywane jako standardowy joystick USB.

## Funkcje
- **3 Pedały:** Gaz, Hamulec, Sprzęgło.
- **8 Przycisków (Shifter):** Obsługa przycisków cyfrowych (np. do skrzyni biegów lub kierownicy).
- **Łatwa Konfiguracja:** Nowoczesna aplikacja PC (ciemny motyw) do ustawienia zakresów Min/Max oraz martwych stref.
- **Wysoka Rozdzielczość:** 16-bitowa precyzja.
- **Zapis Ustawień:** Pamięta kalibrację po restarcie.

## Wymagany Sprzęt
- ESP32-S3 DevKitC-1
- 3x Potencjometr 50k (liniowy)
- Przełączniki/Przyciski (max 8)
- Kabel USB

## Podłączenie (Wiring)

### Pedały (Analogowe)
| Pedał    | Pin ESP32-S3 |
|----------|--------------|
| Gaz      | GPIO 1       |
| Hamulec  | GPIO 2       |
| Sprzęgło | GPIO 3       |

**Uwaga:** Środkowy pin potencjometru (Wiper) podłącz do pinu GPIO, a skrajne do 3.3V i GND.

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

### 2. Konfiguracja Pedałów
1.  Uruchom `run_configurator.bat` (Windows) lub:
    ```bash
    pythonw pedals-configurator/configurator.pyw
    ```
2.  Aplikacja uruchomi się bez okna konsoli. Wybierz port COM i kliknij **Połącz**.
3.  Kalibracja każdego pedału:
    -   Kliknij **Ustaw** (Set) obok Min (puszczony).
    -   Wciśnij pedał do końca, kliknij **Ustaw** obok Max (wciśnięty).
    -   (Opcjonalnie) Ustaw martwe strefy (**Martwa strefa**) w % dla początku i końca zakresu.
4.  Przyciski można przetestować w sekcji **Przyciski / Shifter** (podświetlą się na zielono).
5.  Kliknij **Zapisz Kalibrację w Urządzeniu**.

## Zaawansowana Konfiguracja (Dla Programistów)
Jeśli chcesz modyfikować kod:
1.  Zainstaluj [PlatformIO](https://platformio.org/).
2.  Otwórz folder `pedals-firmware`.
3.  Edytuj `src/main.cpp` (zmień piny, logikę, itp.).
4.  Skompiluj i wgraj (Build and Upload) bezpośrednio z PlatformIO.
