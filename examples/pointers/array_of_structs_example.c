// Test case: Pointer to array of structs
// This tests slice handling with struct elements

struct Sample {
    int id;
    float value;
};

// Calculate average from array of samples
float average_samples(const struct Sample *samples, int count) {
    if (!samples || count <= 0) {
        return 0.0f;
    }

    float sum = 0.0f;
    for (int i = 0; i < count; i++) {
        sum += samples[i].value;
    }

    return sum / count;
}
