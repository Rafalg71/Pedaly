# Pedały SimRacing (ESP32-S3)

Niestandardowe pedały SimRacing używające układu ESP32-S3. Wykrywane jako standardowy joystick USB.

## Funkcje
- **3 Pedały:** Gaz, Hamulec, Sprzęgło.
- **Łatwa Konfiguracja:** Użyj aplikacji PC do ustawienia zakresów Min/Max oraz martwych stref (deadzones).
- **Wysoka Rozdzielczość:** 16-bitowa precyzja.
- **Zapis Ustawień:** Pamięta kalibrację po restarcie.

## Wymagany Sprzęt
- ESP32-S3 DevKitC-1
- 3x Potencjometr 10k
- Kabel USB

## Podłączenie (Wiring)
| Pedał    | Pin ESP32-S3 |
|----------|--------------|
| Gaz      | GPIO 1       |
| Hamulec  | GPIO 2       |
| Sprzęgło | GPIO 3       |

**Uwaga:** Środkowy pin potencjometru (Wiper) podłącz do pinu GPIO, a skrajne do 3.3V i GND.

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
    python pedals-configurator/configurator.py
    ```
2.  Wybierz port COM i kliknij **Connect** (Połącz).
3.  Kalibracja każdego pedału:
    -   Kliknij **Set** obok Min (puszczony).
    -   Wciśnij pedał do końca, kliknij **Set** obok Max (wciśnięty).
    -   (Opcjonalnie) Ustaw martwe strefy (**Deadzone**) w % dla początku i końca zakresu.
4.  Kliknij **Save Calibration** (Zapisz Kalibrację).

## Zaawansowana Konfiguracja (Dla Programistów)
Jeśli chcesz modyfikować kod:
1.  Zainstaluj [PlatformIO](https://platformio.org/).
2.  Otwórz folder `pedals-firmware`.
3.  Edytuj `src/main.cpp` (zmień piny, logikę, itp.).
4.  Skompiluj i wgraj (Build and Upload) bezpośrednio z PlatformIO.
