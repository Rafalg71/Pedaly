@echo off
echo Instalowanie zaleznosci (jesli potrzebne)...
pip install -r pedals-configurator\requirements.txt >nul 2>&1

echo Uruchamianie Konfiguratora...
start "" pythonw pedals-configurator\configurator.pyw
exit
