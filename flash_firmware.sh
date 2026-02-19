#!/bin/bash
# Linux/Mac script to flash firmware

read -p "Enter the device path of your ESP32 (e.g., /dev/ttyACM0): " PORT

if [ -z "$PORT" ]; then
    echo "No port entered. Exiting."
    exit 1
fi

echo "Installing esptool..."
pip install esptool
if [ $? -ne 0 ]; then
    echo "Failed to install esptool. Please ensure pip is installed."
    exit 1
fi

echo "Flashing firmware to $PORT..."
python3 -m esptool --chip esp32s3 --port $PORT --baud 921600 write_flash -z 0x10000 bin/firmware.bin

echo "Done! Reset your board."
