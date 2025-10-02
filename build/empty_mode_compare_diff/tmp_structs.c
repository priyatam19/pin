# 0 "tmp_structs.c"
# 0 "<built-in>"
# 0 "<command-line>"
# 1 "/usr/include/stdc-predef.h" 1 3 4
# 0 "<command-line>" 2
# 1 "tmp_structs.c"

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

int main(void) {
    classify_mode_empty("");
    classify_mode_empty("heat");
    classify_mode_empty("cool");
    classify_mode_empty("fan");
    return 0;
}
