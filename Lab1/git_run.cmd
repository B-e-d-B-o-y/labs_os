@echo off
echo Updating repository...
git add *
git commit -m "Init commit"
if exist build rmdir /s /q build
mkdir build
cd build
cmake ..
cmake --build .
pause
cd ..
app.exe