@echo off
echo Updating repository...
git add *
git commit -m "Init commit"
if exist build (
    rmdir /S /Q build
)
mkdir build
cd build
cmake ..
cmake --build .
cd ..
app.exe

