#include <stdio.h>

int analyze_sensor(int id, double reading, int flag0, int flag1, int flag2, const char *note) {
    int flag_sum = flag0 + flag1 + flag2;
    double adjusted = reading + flag_sum * 0.5;

    if (note && note[0] != '\0') {
        printf("sensor %d note:%s\n", id, note);
    }

    if (adjusted > 100.0) {
        printf("sensor %d overload (%0.2f)\n", id, adjusted);
        return 2;
    }

    if (adjusted < 20.0) {
        printf("sensor %d underflow (%0.2f)\n", id, adjusted);
        return -1;
    }

    printf("sensor %d stable (%0.2f)\n", id, adjusted);
    return 0;
}

#ifdef MIXED_STRUCT_MAIN
int main(void) {
    analyze_sensor(10, 75.5, 1, 0, 1, "routine");
    return 0;
}
#endif