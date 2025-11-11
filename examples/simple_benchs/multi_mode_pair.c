#include <stdio.h>
#include <string.h>

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

#ifdef MULTI_MODE_PAIR_MAIN
int main(void) {
    compare_modes("heat", "");
    compare_modes("cool", "heat");
    return 0;
}
#endif
