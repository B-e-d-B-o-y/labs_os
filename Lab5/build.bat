@echo off
mkdir build 2>nul
cd build
cmake ..
cmake --build .