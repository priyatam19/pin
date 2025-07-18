#!/bin/bash

# Wrapper script to build multiple guestbook variants
# Run as: ./build_all_variants.sh

set -e

# Generate list of variant versions from 0.1.0 to 0.16.7
VARIANT_VERSIONS=()
for MAJOR in {1..16}; do
    for MINOR in {0..7}; do
        VARIANT_VERSIONS+=("0.$MAJOR.$MINOR")
    done
done

for VERSION in "${VARIANT_VERSIONS[@]}"; do
    echo "Building variant: $VERSION"
    ./guestbook_build.sh "$VERSION"
    echo "Finished building variant: $VERSION"
done

echo "All variants built successfully!"