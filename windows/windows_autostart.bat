@echo off

:: Location of the Project .exe file. Will be different for other PC's
set EXE_PATH="C:\Users\joshu\OneDrive\Desktop\Projects\Arduino\wifi-temp-monitor-gui\windows\TemperatureMonitor.exe"

:: Exact location of Startup folder for windows
set SHORTCUT_PATH="%APPDATA%\Microsoft\Windows\Start Menu\Programs\Startup\TemperatureMonitor.lnk"

:: Check if shortcut exists. If not, then automatically create shortcut in Startup folder
if exist %SHORTCUT_PATH% (
  echo Shortcut already exists.
) else (
  echo Creating shortcut...
  powershell -Command "$s=(New-Object -COM WScript.Shell).CreateShortcut('%SHORTCUT_PATH%'); $s.TargetPath='%EXE_PATH%'; $s.Save()"
  echo Shortcut created successfully.
)
