#!/bin/bash

# Absolute paths
CPP_FILE="/home/ufg/anaconda3/envs/fa_5glena/myproject/shared_memory/shared_memory_writer.cc"
BUILD_DIR="/home/ufg/anaconda3/envs/fa_5glena/myproject/shared_memory/build"
SCRIPT_PATH=$(readlink -f "$0")

# Ensure this script has the correct executable permissions
if [ ! -x "$SCRIPT_PATH" ]; then
    echo "Setting executable permissions for the script..."
    chmod +x "$SCRIPT_PATH"
fi

# Ensure the build directory exists
echo "Ensuring build directory exists at: $BUILD_DIR"
mkdir -p $BUILD_DIR

# Check if required tools are installed
if ! command -v python3 &> /dev/null; then
    echo "Error: Python3 is not installed. Please install Python3."
    exit 1
fi

if ! python3 -m pybind11 --includes &> /dev/null; then
    echo "Error: Pybind11 is not installed. Please install it using 'pip install pybind11'."
    exit 1
fi

if ! command -v c++ &> /dev/null; then
    echo "Error: C++ compiler (c++) is not installed. Please install it."
    exit 1
fi

# Recompile the C++ code and generate the shared object (.so) file
echo "Compiling $CPP_FILE to shared object in $BUILD_DIR..."
c++ -O3 -Wall -shared -std=c++17 -fPIC `python3 -m pybind11 --includes` \
    $CPP_FILE -o $BUILD_DIR/shared_memory_writer`python3-config --extension-suffix`

# Check if the compilation was successful
if [ $? -eq 0 ]; then
  echo "Writer code recompiled and shared object updated successfully!"
else
  echo "Compilation failed. Please check for errors in your code or dependencies."
  exit 1
fi

