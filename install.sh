#!/usr/bin/env bash

fail() {
    printf "\n%s\n\n" "[ERROR] $1"
    exit 1
}

# Get the directory of the script
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &>/dev/null && pwd )"

# Create build directory
mkdir -p "$SCRIPT_DIR/build"

# Configure the project with CMake
cmake -S "$SCRIPT_DIR" \
      -B "build" \
      -DCMAKE_BUILD_TYPE=Release \
      -DBUILD_SHARED_LIBS=ON \
      -DBUILD_TESTING=OFF \
      -DWITH_COVERAGE=OFF \
      -DWITH_ASAN=OFF \
      -DWITH_TSAN=OFF

# Change into the build directory
cd "build" || fail "Failed to cd into the \"build\" directory."

make "-j$(nproc --all)"
sudo make install
