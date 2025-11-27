@echo off
echo Starting two processes...
echo ================================
start "First" /D "out/build/x64-Release" lw5.exe
start "Second" /D "out/build/x64-Release" lw5.exe
exit