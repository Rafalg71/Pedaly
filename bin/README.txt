This directory contains the pre-compiled firmware for the ESP32-S3 Pedals.

firmware.bin: The application firmware.

To flash:
Run 'flash_firmware.bat' (Windows) or 'flash_firmware.sh' (Linux/Mac) in the root directory.

To build manually:
1. Open pedals-firmware in PlatformIO
2. Run 'Build'
3. The binary will be in .pio/build/esp32-s3-devkitc-1/firmware.bin
