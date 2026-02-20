REM Windows Batch Script to flash firmware
@echo off
set /p COM=Wpisz port COM swojego ESP32 (np. COM5):

if "%COM%"=="" (
    echo Nie podano portu COM. Konczenie.
    pause
    exit /b
)

echo Instalowanie esptool...
pip install esptool
if %ERRORLEVEL% NEQ 0 (
    echo Nie udalo sie zainstalowac esptool. Upewnij sie, ze Python jest zainstalowany.
    pause
    exit /b
)

echo.
echo =======================================================
echo Wazne: Jesli wgrywanie sie zawiesi na 'Serial port...',
echo prosze przytrzymac przycisk BOOT na plytce ESP32.
echo =======================================================
echo.

echo Wgrywanie firmware do %COM%...
python -m esptool --chip esp32s3 --port %COM% --baud 460800 --before default_reset --after hard_reset write_flash -z 0x10000 bin\firmware.bin

echo.
echo Gotowe! Zresetuj urzadzenie przyciskiem RST.
pause
