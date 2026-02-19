@echo off
echo Installing dependencies...
pip install -r pedals-configurator\requirements.txt
if %ERRORLEVEL% NEQ 0 (
    echo Failed to install dependencies. Please ensure Python is installed and added to PATH.
    pause
    exit /b
)

echo Starting Configurator...
python pedals-configurator\configurator.py
pause
