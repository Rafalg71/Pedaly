REM Windows Batch Script to flash firmware
@echo off
set /p COM=Enter the COM port of your ESP32 (e.g., COM5):

if "%COM%"=="" (
    echo No COM port entered. Exiting.
    pause
    exit /b
)

echo Installing esptool...
pip install esptool
if %ERRORLEVEL% NEQ 0 (
    echo Failed to install esptool. Please ensure Python is installed and added to PATH.
    pause
    exit /b
)

echo Flashing firmware to %COM%...
python -m esptool --chip esp32s3 --port %COM% --baud 921600 write_flash -z 0x10000 bin\firmware.bin

echo Done! Reset your board.
pause
