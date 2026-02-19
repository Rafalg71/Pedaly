@echo off
echo Instalowanie zaleznosci (dependencies)...
pip install -r pedals-configurator\requirements.txt
if %ERRORLEVEL% NEQ 0 (
    echo Nie udalo sie zainstalowac zaleznosci. Upewnij sie, ze Python jest w PATH.
    pause
    exit /b
)

echo Uruchamianie Konfiguratora...
python pedals-configurator\configurator.py
pause
