// Test case: Void pointer (opaque pointer)
// This tests fallback handling for generic pointers

// Generic data processor using void pointer
int process_generic(void *data, int data_type) {
    if (!data) {
        return -1;
    }

    // In real code, would cast based on data_type
    // For testing, just return success
    return data_type;
}
