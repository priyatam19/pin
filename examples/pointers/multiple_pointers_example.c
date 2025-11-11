// Test case: Multiple pointer parameters of different types
// This tests handling of mixed pointer types in a single function

// Process data with multiple pointer inputs
int process_data(
    int *error_code,           // Output: scalar pointer
    const float *readings,     // Input: array pointer
    int num_readings,          // Length companion
    const char *label          // Input: string pointer
) {
    if (!readings || num_readings <= 0) {
        if (error_code) {
            *error_code = -1;
        }
        return 0;
    }

    float sum = 0.0f;
    for (int i = 0; i < num_readings; i++) {
        sum += readings[i];
    }

    if (error_code) {
        *error_code = 0;
    }

    return (int)sum;
}
