#include <stdio.h>

int dump_mode_bytes(const char *mode) {
    if (mode == NULL) {
        puts("mode:null");
        return -1;
    }

    puts("mode-bytes:");
    for (int i = 0; i < 8; ++i) {
        unsigned char byte = (unsigned char)mode[i];
        printf("[%d]=0x%02x\n", i, byte);
        if (byte == '\0') {
            break;
        }
    }
    return 0;
}

#ifdef MODE_BUFFER_DUMP_MAIN
int main(void) {
    dump_mode_bytes("heat");
    return 0;
}
#endif
