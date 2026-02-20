#!/bin/bash
# Linux/Mac script to flash firmware

read -p "Wpisz sciezke portu ESP32 (np. /dev/ttyACM0): " PORT

if [ -z "$PORT" ]; then
    echo "Nie podano portu. Konczenie."
    exit 1
fi

echo "Instalowanie esptool..."
pip install esptool
if [ $? -ne 0 ]; then
    echo "Nie udalo sie zainstalowac esptool."
    exit 1
fi

echo ""
echo "======================================================="
echo "Wazne: Jesli wgrywanie sie zawiesi na 'Serial port...',"
echo "prosze przytrzymac przycisk BOOT na plytce ESP32."
echo "======================================================="
echo ""

echo "Wgrywanie firmware do $PORT..."
python3 -m esptool --chip esp32s3 --port $PORT --baud 460800 --before default_reset --after hard_reset write_flash -z 0x10000 bin/firmware.bin

echo ""
echo "Gotowe! Zresetuj urzadzenie przyciskiem RST."
