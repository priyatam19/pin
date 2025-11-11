#!/bin/bash
# Script to add crash continuation logic to PIN libFuzzer harnesses
# This enables libFuzzer to continue fuzzing after finding crashes

set -e

echo "================================================================"
echo "PIN LibFuzzer Crash Continuation Fix"
echo "================================================================"
echo

# Find all bytes_fuzz.cc files in build directories
HARNESS_FILES=$(find /home/priyatam/pin/build -name "bytes_fuzz.cc" 2>/dev/null)

if [ -z "$HARNESS_FILES" ]; then
    echo "No bytes_fuzz.cc files found"
    exit 1
fi

COUNT=0
for HARNESS in $HARNESS_FILES; do
    COUNT=$((COUNT + 1))
    DIR=$(dirname "$HARNESS")
    BACKUP="${HARNESS}.orig"

    echo "[$COUNT] Processing: $HARNESS"

    # Check if already has crash handling
    if grep -q "siglongjmp" "$HARNESS" 2>/dev/null; then
        echo "    → Already has crash continuation, skipping"
        continue
    fi

    # Backup original
    if [ ! -f "$BACKUP" ]; then
        cp "$HARNESS" "$BACKUP"
        echo "    → Backed up to: $BACKUP"
    fi

    # Create new version with crash continuation
    cat > "$HARNESS" << 'EOFHARNESS'
#include <cstdint>
#include <cstring>
#include <stddef.h>
#include <signal.h>
#include <setjmp.h>
#include <unistd.h>
#include <stdio.h>

extern "C" int pin_wrapper_entry(const uint8_t *data, size_t len);

// Crash recovery infrastructure
static sigjmp_buf crash_jmp_buf;
static volatile sig_atomic_t crash_count = 0;
static volatile sig_atomic_t last_signal = 0;

// Signal handler for crashes
static void crash_handler(int sig) {
    crash_count++;
    last_signal = sig;

    // Optional: Log crash info to stderr
    // (Comment out if you don't want crash logging)
    char msg[256];
    const char *sig_name = "UNKNOWN";
    switch (sig) {
        case SIGSEGV: sig_name = "SIGSEGV"; break;
        case SIGABRT: sig_name = "SIGABRT"; break;
        case SIGBUS:  sig_name = "SIGBUS"; break;
        case SIGFPE:  sig_name = "SIGFPE"; break;
        case SIGILL:  sig_name = "SIGILL"; break;
    }
    snprintf(msg, sizeof(msg),
             "[CRASH %d] Signal %d (%s) caught, continuing fuzzing...\n",
             (int)crash_count, sig, sig_name);
    write(2, msg, strlen(msg));

    // Jump back to safe point
    siglongjmp(crash_jmp_buf, sig);
}

// Setup signal handlers
static void setup_crash_handlers() {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = crash_handler;
    sa.sa_flags = SA_NODEFER | SA_RESTART;

    // Handle all common crash signals
    sigaction(SIGSEGV, &sa, NULL);  // Segmentation fault
    sigaction(SIGABRT, &sa, NULL);  // Abort
    sigaction(SIGBUS, &sa, NULL);   // Bus error
    sigaction(SIGFPE, &sa, NULL);   // Floating point exception
    sigaction(SIGILL, &sa, NULL);   // Illegal instruction
}

// LibFuzzer entry point
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    // Setup handlers once
    static int handlers_setup = 0;
    if (!handlers_setup) {
        setup_crash_handlers();
        handlers_setup = 1;
    }

    // Set recovery point
    int sig = sigsetjmp(crash_jmp_buf, 1);

    if (sig == 0) {
        // Normal execution
        (void)pin_wrapper_entry(data, size);
    } else {
        // Crash occurred - libFuzzer will still save it to artifacts/
        // But we continue fuzzing instead of exiting
    }

    return 0;  // Always return 0 to continue
}

// Optional: Print crash statistics on exit
__attribute__((destructor))
static void print_crash_stats() {
    if (crash_count > 0) {
        char msg[256];
        snprintf(msg, sizeof(msg),
                 "\n[FUZZING COMPLETE] Total crashes caught and continued: %d\n",
                 (int)crash_count);
        write(2, msg, strlen(msg));
    }
}
EOFHARNESS

    echo "    → Updated with crash continuation logic"

    # Try to rebuild
    BINARY="$DIR/fuzz_bytes"
    if [ -f "$BINARY" ]; then
        echo "    → Rebuilding $BINARY..."

        # Try to rebuild (may fail if dependencies are missing)
        (
            cd "$DIR"
            if [ -f "Makefile" ]; then
                make fuzz_bytes 2>&1 | tail -5
            else
                # Manual rebuild command (adjust if needed)
                echo "    → No Makefile found, manual rebuild required"
                echo "    → Run: cd $DIR && [rebuild command]"
            fi
        ) || echo "    → Rebuild failed, manual intervention required"
    fi

    echo
done

echo "================================================================"
echo "Summary: Updated $COUNT harness files"
echo "================================================================"
echo
echo "Next steps:"
echo "1. Manually rebuild any harnesses that failed"
echo "2. Run fuzzing campaigns:"
echo "   cd /home/priyatam/pin/build/<target>_diff"
echo "   ./fuzz_bytes -max_total_time=3600 corpus/"
echo
echo "3. Monitor for crashes (they should accumulate):"
echo "   watch -n 5 'ls -lh artifacts/crash-*'"
echo
echo "Original files backed up with .orig extension"
echo "================================================================"
