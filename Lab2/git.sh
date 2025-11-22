#!/bin/bash
echo "Updating repository..."
git add *
git commit -m "Init commit"
if [ -d "build" ]; then
	rm -rf "build";
fi
mkdir build
cd build
cmake ..
cmake --build .