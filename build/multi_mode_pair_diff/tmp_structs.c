# 0 "tmp_structs.c"
# 0 "<built-in>"
# 0 "<command-line>"
# 1 "/usr/include/stdc-predef.h" 1 3 4
# 0 "<command-line>" 2
# 1 "tmp_structs.c"

int compare_modes(const char *primary, const char *secondary) {
    if (!primary || !secondary) {
        puts("mode:null");
        return -1;
    }

    printf("primary:%s secondary:%s\n", primary, secondary);

    if (primary[0] == '\0') {
        puts("primary-empty");
    }
    if (secondary[0] == '\0') {
        puts("secondary-empty");
    }
    if (strcmp(primary, secondary) == 0) {
        puts("modes:equal");
    }
    return 0;
}

int main(void) {
    compare_modes("heat", "");
    compare_modes("cool", "heat");
    return 0;
}
