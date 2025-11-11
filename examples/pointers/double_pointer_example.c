// Test case: Double pointer (nullable pointer to pointer)
// This tests pointer-to-pointer handling

struct Config {
    int timeout;
    int retries;
};

// Nullable pointer to pointer - common in APIs that may optionally return a value
int get_config(struct Config **out_config) {
    if (!out_config) {
        return -1;  // Invalid argument
    }

    // Simulate: sometimes we have config, sometimes we don't
    // For testing, we'll just return success
    return 0;
}
