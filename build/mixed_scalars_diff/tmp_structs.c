# 0 "tmp_structs.c"
# 0 "<built-in>"
# 0 "<command-line>"
# 1 "/usr/include/stdc-predef.h" 1 3 4
# 0 "<command-line>" 2
# 1 "tmp_structs.c"

int evaluate_sensor(int sensor_id, float temperature, const char *mode) {
    float threshold;
    if (mode == NULL) {
        printf("sensor %d mode-missing\n", sensor_id);
        return -1;
    }

    if (strcmp(mode, "heat") == 0) {
        threshold = 75.0f;
    } else if (strcmp(mode, "cool") == 0) {
        threshold = 45.0f;
    } else {
        threshold = 60.0f;
    }

    if (temperature > threshold) {
        printf("sensor %d high (%0.2f > %0.2f)\n", sensor_id, temperature, threshold);
        return 1;
    }

    printf("sensor %d normal (%0.2f <= %0.2f)\n", sensor_id, temperature, threshold);
    return 0;
}

int main(void) {
    evaluate_sensor(1, 55.0f, "heat");
    evaluate_sensor(2, 38.0f, "cool");
    evaluate_sensor(3, 80.0f, "heat");
    return 0;
}
