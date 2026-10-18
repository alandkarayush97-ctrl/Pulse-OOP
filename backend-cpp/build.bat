@echo off
cmake -S . -B build || exit /b 1
cmake --build build --config Release || exit /b 1
if not exist .env copy .env.example .env
echo.
echo Built. Run:  build\Release\pulse_server.exe
