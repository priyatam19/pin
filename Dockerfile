# syntax=docker/dockerfile:1
FROM ubuntu:22.04

ARG DEBIAN_FRONTEND=noninteractive

# Core build dependencies and tooling for PIN pipelines
RUN apt-get update && apt-get install -y \
    build-essential \
    python3 \
    python3-pip \
    python3-venv \
    python3-clang \
    git \
    curl \
    protobuf-compiler \
    libprotobuf-dev \
    pkg-config \
    clang \
    libclang-dev \
    llvm \
    binutils \
    ca-certificates \
    unzip \
    && rm -rf /var/lib/apt/lists/*

# Upgrade pip and install Python packages with versions known to work with libprotoc 3.12
RUN python3 -m pip install --upgrade pip \
    && python3 -m pip install --no-cache-dir \
        pycparser \
        protobuf==3.20.3 \
        nanopb==0.4.7

# Ensure the nanopb protoc plugin is on PATH for protoc --nanopb_out invocations
RUN python3 - <<'PY'
from pathlib import Path
import nanopb
plugin = Path(nanopb.__file__).parent / "generator" / "protoc-gen-nanopb"
target = Path("/usr/local/bin/protoc-gen-nanopb")
if not plugin.exists():
    raise SystemExit(f"Expected nanopb plugin at {plugin}, but it was not found")
if target.exists():
    target.unlink()
target.symlink_to(plugin)
print(f"Linked nanopb plugin: {target} -> {plugin}")
PY

# Provide a ready-to-copy nanopb checkout for C sources when the repo bind mount lacks them
RUN git clone --depth 1 https://github.com/nanopb/nanopb.git /opt/nanopb

# Set up a workspace directory and keep PATH tidy for tooling
WORKDIR /workspace
ENV PATH=/usr/local/bin:$PATH

# Lightweight entrypoint auto-populates /workspace/nanopb if empty (can disable via PIN_AUTO_POPULATE_NANOPB=0)
RUN cat <<'EOF_ENTRY' > /usr/local/bin/pin-entrypoint.sh
#!/bin/bash
set -e

WORKSPACE_DIR="${PIN_WORKSPACE_DIR:-/workspace}"
TARGET_DIR="$WORKSPACE_DIR/nanopb"
SOURCE_DIR="/opt/nanopb"

if [[ "${PIN_AUTO_POPULATE_NANOPB:-1}" == "1" ]] && [[ -d "$SOURCE_DIR" ]]; then
    mkdir -p "$WORKSPACE_DIR"
    if [[ ! -d "$TARGET_DIR" ]]; then
        cp -a "$SOURCE_DIR" "$TARGET_DIR"
    else
        if [[ -z "$(find "$TARGET_DIR" -mindepth 1 -maxdepth 1 -print -quit)" ]]; then
            cp -a "$SOURCE_DIR"/. "$TARGET_DIR"/
        fi
    fi
fi

exec "$@"
EOF_ENTRY
RUN chmod +x /usr/local/bin/pin-entrypoint.sh

ENTRYPOINT ["/usr/local/bin/pin-entrypoint.sh"]
# Default command drops into bash so users can run pipeline scripts manually
CMD ["/bin/bash"]
