#include <stdio.h>
#include <string.h>

int classify_mode_empty(const char *mode) {
    if (mode == NULL) {
        puts("mode:null");
        return -1;
    }
    if (mode[0] == '\0') {
        puts("mode:empty");
        return 0;
    }
    if (strcmp(mode, "heat") == 0) {
        puts("mode:heat");
        return 1;
    }
    if (strcmp(mode, "cool") == 0) {
        puts("mode:cool");
        return 2;
    }
    printf("mode:other:%s\n", mode);
    return 3;
}

#ifdef EMPTY_MODE_COMPARE_MAIN
int main(void) {
    classify_mode_empty("");
    classify_mode_empty("heat");
    classify_mode_empty("cool");
    classify_mode_empty("fan");
    return 0;
}
#endif
