#!/bin/bash

# Script to build the guestbook project locally on Ubuntu 24.04 LTS
# Sets up vcpkg and builds a specified variant
# Run as: ./build_guestbook.sh [variant_version]
# Example: ./build_guestbook.sh 0.1.0

set -e  # Exit on error

# Accept variant version as an argument (default to 0.1.0 if not provided)
VARIANT_VERSION=${1:-"0.1.0"}

# Define directories and environment variables
WORK_DIR="/home/priyatam/pin/guestbook/guestbook-builds-eval1"
VCPKG_ROOT="$WORK_DIR/vcpkg"
TOOLCHAIN_FILE="$WORK_DIR/opt/toolchain/cromulence-toolchain.cmake"
TRIPLET="x64-linux-cromulence"
VCPKG_OVERLAY_PORTS="$WORK_DIR/variant-builds/$VARIANT_VERSION/vcpkg-overlays/ports"
VCPKG_OVERLAY_TRIPLETS="$WORK_DIR/variant-builds/$VARIANT_VERSION/vcpkg-overlays/triplets"
VARIANT_DIR="$WORK_DIR/variant-builds/$VARIANT_VERSION"
APP_DIR="$VARIANT_DIR/app"
VIEWS_DIR="$VARIANT_DIR/views"
BUILD_DIR="$VARIANT_DIR/build"

# Step 1: Verify required files
echo "Verifying required files for variant $VARIANT_VERSION..."
REQUIRED_FILES=(
    "$APP_DIR/CMakeLists.txt"
    "$APP_DIR/vcpkg.json"
    "$APP_DIR/src/assert.cpp"
    "$APP_DIR/src/base64.cpp"
    "$APP_DIR/src/db.cpp"
    "$APP_DIR/src/session.cpp"
    "$APP_DIR/src/template_finder.cpp"
    "$APP_DIR/src/util_rand.cpp"
    "$APP_DIR/src/main.cpp"
    "$APP_DIR/src/tpl/filter.cpp"
    "$APP_DIR/src/tpl/index.cpp"
    "$VIEWS_DIR/filter.html.mustache"
    "$VIEWS_DIR/index.html.mustache"
    "$VCPKG_OVERLAY_PORTS/sqlite3/CMakeLists.txt"
    "$VCPKG_OVERLAY_PORTS/sqlite3/vcpkg.json"
    "$VCPKG_OVERLAY_PORTS/sqlite3/fix-arm-uwp.patch"
    "$VCPKG_OVERLAY_PORTS/sqlite3/add-config-include.patch"
    "$VCPKG_OVERLAY_PORTS/sqlite3/sqlite3.pc.in"
    "$VCPKG_OVERLAY_PORTS/sqlite3/sqlite3-vcpkg-config.h.in"
    "$VCPKG_OVERLAY_PORTS/sqlite3/sqlite3-config.in.cmake"
    "$VCPKG_OVERLAY_PORTS/sqlite3/usage"
    "$VCPKG_OVERLAY_PORTS/crow/portfile.cmake"
    "$VCPKG_OVERLAY_PORTS/crow/vcpkg.json"
    "$VCPKG_OVERLAY_TRIPLETS/x64-linux-cromulence.cmake"
    "$TOOLCHAIN_FILE"
)
for file in "${REQUIRED_FILES[@]}"; do
    if [[ ! -f "$file" ]]; then
        echo "Error: Required file $file is missing!"
        exit 1
    fi
done

# Step 2: Write the triplet file with the correct toolchain path
echo "Writing triplet file with the correct toolchain path..."
cat << EOF > "$VCPKG_OVERLAY_TRIPLETS/x64-linux-cromulence.cmake"
set(VCPKG_TARGET_ARCHITECTURE x64)
set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE static)
set(VCPKG_CMAKE_SYSTEM_NAME Linux)
set(VCPKG_CHAINLOAD_TOOLCHAIN_FILE $TOOLCHAIN_FILE)
set(VCPKG_BUILD_TYPE release)
EOF

# Step 3: Install system packages
echo "Installing system packages..."
sudo apt-get update
sudo apt-get install -y --no-install-recommends \
    build-essential \
    clang \
    lld \
    cmake \
    ninja-build \
    git \
    make \
    wget \
    curl \
    zip \
    unzip \
    time \
    ca-certificates \
    pkg-config \
    libicu-dev \
    g++ \
    libstdc++-12-dev \
    autoconf \
    automake \
    libtool \
    autoconf-archive
sudo apt-get clean
sudo rm -rf /var/lib/apt/lists/*

# Step 4: Set up vcpkg
echo "Setting up vcpkg..."
if [[ ! -d "$VCPKG_ROOT" || ! -f "$VCPKG_ROOT/vcpkg" ]]; then
    echo "vcpkg not found or incomplete, cloning repository..."
    rm -rf "$VCPKG_ROOT"  # Clean up any incomplete installation
    mkdir -p "$VCPKG_ROOT"
    git clone --depth 1 --branch 2025.03.19 https://github.com/microsoft/vcpkg.git "$VCPKG_ROOT"
    "$VCPKG_ROOT/bootstrap-vcpkg.sh" -disableMetrics
    chown -R "$USER:$USER" "$VCPKG_ROOT"
else
    echo "vcpkg already exists at $VCPKG_ROOT, skipping clone..."
fi

# Step 5: Set environment variables
echo "Setting environment variables..."
export PATH="$VCPKG_ROOT:$PATH"
export VCPKG_ROOT="$VCPKG_ROOT"
export TOOLCHAIN_FILE="$TOOLCHAIN_FILE"
export TRIPLET="$TRIPLET"
export VCPKG_OVERLAY_PORTS="$VCPKG_OVERLAY_PORTS"
export VCPKG_OVERLAY_TRIPLETS="$VCPKG_OVERLAY_TRIPLETS"

# Persist environment variables
cat << EOF >> "$HOME/.bashrc"
export PATH=$VCPKG_ROOT:\$PATH
export VCPKG_ROOT=$VCPKG_ROOT
export TOOLCHAIN_FILE=$TOOLCHAIN_FILE
export TRIPLET=$TRIPLET
export VCPKG_OVERLAY_PORTS=$VCPKG_OVERLAY_PORTS
export VCPKG_OVERLAY_TRIPLETS=$VCPKG_OVERLAY_TRIPLETS
EOF

# Step 6: Install dependencies with vcpkg
echo "Installing dependencies with vcpkg..."
cd "$VCPKG_ROOT"
export CC=clang
export CXX=clang++
export LD=lld
./vcpkg install sqlite3[fts5,math,unicode,dbstat,fts3,fts4,rtree,session]:$TRIPLET crow:$TRIPLET libsodium:$TRIPLET

# Step 7: Configure and build the project
echo "Configuring and building variant $VARIANT_VERSION..."
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"
cmake -S "$APP_DIR" -B . \
    -DCMAKE_TOOLCHAIN_FILE="$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake" \
    -DVCPKG_CHAINLOAD_TOOLCHAIN_FILE="$TOOLCHAIN_FILE" \
    -DVCPKG_TARGET_TRIPLET="$TRIPLET" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_CXX_FLAGS_RELEASE="-UNDEBUG" 
cmake --build . --target guestbook --verbose

# Step 8: Set up runtime environment
echo "Setting up runtime environment..."
ln -sf "$VIEWS_DIR" "$BUILD_DIR/views"

# Step 9: Output instructions
echo "Build complete for variant $VARIANT_VERSION!"
echo "You can run the application with:"
echo "  cd $BUILD_DIR"
echo "  ./bin/guestbook"
echo "The executable is located at: $BUILD_DIR/bin/guestbook"