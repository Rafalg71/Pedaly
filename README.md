# SimRacing Pedals (ESP32-S3)

This project contains the firmware and configuration software for custom SimRacing pedals using an ESP32-S3 WROOM-1 board.

## Features
- **3 Potentiometers:** Throttle, Brake, Clutch.
- **USB HID Joystick:** Recognized as a standard game controller by Windows/Simulators.
- **High Resolution:** 12-bit ADC mapped to 16-bit HID axis (0-65535).
- **Configurable:** Set Min/Max calibration values via the included PC app.
- **Persistent Settings:** Calibration data is saved in ESP32's non-volatile memory (NVS).

## Directory Structure
- `pedals-firmware/`: PlatformIO project for the ESP32-S3.
- `pedals-configurator/`: Python application to configure the pedals.

## Hardware Requirements
- **Microcontroller:** ESP32-S3 DevKitC-1 or compatible module.
- **Sensors:** 3x 10k Linear Potentiometers.
- **Connection:** USB Data Cable (connected to the Native USB port of the ESP32-S3).

## Pinout & Wiring

| Component | ESP32-S3 Pin | Function    |
|-----------|--------------|-------------|
| Throttle  | GPIO 1       | Analog In   |
| Brake     | GPIO 2       | Analog In   |
| Clutch    | GPIO 3       | Analog In   |

**Potentiometer Wiring:**
1.  **Pin 1:** 3.3V
2.  **Pin 2 (Wiper):** ESP32 GPIO Pin (1, 2, or 3)
3.  **Pin 3:** GND

*Note: If the pedal works in reverse, you can either swap Pin 1 and 3 physically, or invert the calibration in the software (set Min to the pressed value and Max to the released value).*

## Getting Started

### 1. Firmware Setup
1. Install [PlatformIO](https://platformio.org/) (usually as a VSCode extension).
2. Open the `pedals-firmware` folder in PlatformIO.
3. Connect your ESP32-S3 via the **Native USB** port (often labeled USB or OTG).
4. Build and Upload the firmware.
   - *Note: If the device is not recognized, hold the BOOT button while plugging in to enter bootloader mode.*

### 2. Software Setup
1. Install Python 3.x.
2. Install dependencies:
   ```bash
   pip install -r pedals-configurator/requirements.txt
   ```
3. Run the Configurator:
   ```bash
   python pedals-configurator/configurator.py
   ```

## Calibration Guide
1. Open the Configurator App and connect to the ESP32-S3 COM port.
2. You will see live bars for Throttle, Brake, and Clutch.
3. **Calibrate Throttle:**
   - Leave the pedal released. Click **Set** next to "Min (Released)".
   - Press the pedal fully. Click **Set** next to "Max (Pressed)".
4. Repeat for Brake and Clutch.
5. Click **Save Calibration** to store the settings on the device.

## Serial Protocol (Advanced)
The device communicates over USB Serial (CDC) at 115200 baud.

- `READ`: Returns raw ADC values.
  - Response: `RAW:<throttle>,<brake>,<clutch>` (e.g., `RAW:1200,400,0`)
- `GET_CONFIG`: Returns current calibration.
  - Response: `CONF:<t_min>:<t_max>,<b_min>:<b_max>,<c_min>:<c_max>`
- `SET <idx> <min> <max>`: Sets calibration for pedal `idx` (0=Throttle, 1=Brake, 2=Clutch).
  - Response: `OK`
- `SAVE`: Saves current configuration to NVS.
  - Response: `SAVED`
