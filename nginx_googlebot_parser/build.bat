@echo off
cd /d "%~dp0"
g++ -std=c++17 -O3 -Wall -o nginx_googlebot_parser.exe parser.cpp
if %ERRORLEVEL% equ 0 echo Build OK: nginx_googlebot_parser.exe
