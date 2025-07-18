#!/bin/bash

# Exit on error
set -e

# Store the original working directory
original_dir=$(pwd)

# Check if fuzz_guestbook.cc exists
if [ ! -f "fuzz_guestbook.cc" ]; then
    echo "Error: fuzz_guestbook.cc not found in current directory"
    exit 1
fi

# Ensure variant-builds directory exists
if [ ! -d "variant-builds" ]; then
    echo "Error: variant-builds directory not found"
    exit 1
fi

# Generate variant list: 0.1.0 to 0.16.7
variants=()
for major in $(seq 1 16); do
    for minor in $(seq 0 7); do
        variants+=("0.$major.$minor")
    done
done

# Process each variant
for variant_name in "${variants[@]}"; do
    variant_dir="variant-builds/$variant_name"
    if [ ! -d "$variant_dir" ]; then
        echo "Warning: variant directory $variant_dir not found, skipping"
        continue
    fi

    echo "Processing variant: $variant_name"

    # Check if build directory exists
    build_dir="$variant_dir/build"
    if [ ! -d "$build_dir" ]; then
        echo "Warning: build directory not found for variant $variant_name, skipping"
        continue
    fi

    # Check if app/src directory exists
    app_src_dir="$variant_dir/app/src"
    if [ ! -d "$app_src_dir" ]; then
        echo "Warning: app/src directory not found for variant $variant_name, skipping"
        continue
    fi

    # Check if vcpkg include directory exists
    vcpkg_include="$build_dir/vcpkg_installed/x64-linux-cromulence/include"
    if [ ! -d "$vcpkg_include" ]; then
        echo "Warning: vcpkg include directory ($vcpkg_include) not found for variant $variant_name, skipping"
        continue
    fi

    # Check if crow.h exists
    if [ ! -f "$vcpkg_include/crow.h" ]; then
        echo "Warning: crow.h not found at $vcpkg_include/crow.h for variant $variant_name, skipping"
        continue
    fi

    # Change to variant directory
    cd "$variant_dir"

    # Copy fuzz_guestbook.cc to variant directory
    cp "$original_dir/fuzz_guestbook.cc" .

    # Create seeds directory and all seed files
    mkdir -p seeds
    cat << EOF > seeds/seed_http_info
GET /info HTTP/1.1
Host: localhost
EOF
    cat << EOF > seeds/seed_http_zzz
GET /zzz HTTP/1.1
Host: x
EOF
    cat << EOF > seeds/seed_http_schema
GET /schema HTTP/1.1
Host: x
EOF
    printf '\x00' > seeds/seed0
    printf '\x01' > seeds/seed1
    printf '\x02where[id]=1' > seeds/seed2
    printf '\x03Alice|alice@example.com|Hello' > seeds/seed3
    printf '\x04' > seeds/seed4

    # Collect object files from variant's build directory, excluding src/main.cpp.o
    objs=$(find build -name '*.o' | grep -v 'src/main.cpp.o' | tr '\n' ' ')
    if [ -z "$objs" ]; then
        echo "Warning: No object files found in $build_dir (excluding src/main.cpp.o), skipping variant $variant_name"
        cd "$original_dir"
        continue
    fi

    echo "Compiling fuzz driver for variant: $variant_name"

    # Compile fuzz driver
    afl-clang-fast++ -std=c++17 -O2 -fsanitize=address \
        -Ibuild/vcpkg_installed/x64-linux-cromulence/include \
        -Iapp/src \
        fuzz_guestbook.cc \
        ${objs} \
        -Lbuild/vcpkg_installed/x64-linux-cromulence/lib \
        -lsqlite3 -lsodium -lpthread \
        -o fuzz_guestbook

    echo "Starting fuzzing for variant: $variant_name"

    # Run afl-fuzz (will be limited by concurrency control below)
    afl-fuzz -i seeds -o findings ./fuzz_guestbook &

    # Limit to 4 concurrent fuzzing jobs
    while [ $(jobs -r | wc -l) -ge 4 ]; do
        sleep 1
    done

    # Return to original directory
    cd "$original_dir"
done

# Wait for all fuzzing jobs to complete
wait

echo "Fuzzing completed for all variants. Results are in each variant's 'findings' directory."