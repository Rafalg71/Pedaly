# SimRacing Pedals (ESP32-S3)

Custom SimRacing pedals using an ESP32-S3. Recognized as a USB Joystick.

## Features
- **3 Pedals:** Throttle, Brake, Clutch.
- **Easy Config:** Use the PC app to set Min/Max ranges.
- **High Res:** 16-bit precision.
- **Save Settings:** Remembers calibration after restart.

## Hardware
- ESP32-S3 DevKitC-1
- 3x 10k Potentiometers
- USB Cable

## Wiring
| Pedal    | ESP32-S3 Pin |
|----------|--------------|
| Throttle | GPIO 1       |
| Brake    | GPIO 2       |
| Clutch   | GPIO 3       |

**Note:** Connect Potentiometer Wiper to the GPIO pin, and ends to 3.3V and GND.

## Installation (Easy Way)

### 1. Flash Firmware
1.  Download the latest Release (including `firmware.bin` in the `bin` folder).
2.  Install [Python](https://www.python.org/downloads/).
3.  Connect your ESP32-S3 via USB.
    *   *If not recognized, hold BOOT while plugging in.*
4.  Run `flash_firmware.bat` (Windows) or `./flash_firmware.sh` (Mac/Linux).
5.  Follow the prompts to enter your COM port (e.g., `COM3` or `/dev/ttyACM0`).

### 2. Configure Pedals
1.  Run `run_configurator.bat` (Windows) or:
    ```bash
    python pedals-configurator/configurator.py
    ```
2.  Select your COM port and click **Connect**.
3.  Calibrate each pedal:
    -   Click **Set** next to Min (released).
    -   Press pedal fully, click **Set** next to Max (pressed).
4.  Click **Save Calibration**.

## Advanced Setup (Developer)
If you want to modify the code:
1.  Install [PlatformIO](https://platformio.org/).
2.  Open the `pedals-firmware` folder.
3.  Edit `src/main.cpp` (change pins, logic, etc.).
4.  Build and Upload directly from PlatformIO.
