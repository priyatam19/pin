# 0 "tmp_structs.c"
# 0 "<built-in>"
# 0 "<command-line>"
# 1 "/usr/include/stdc-predef.h" 1 3 4
# 0 "<command-line>" 2
# 1 "tmp_structs.c"

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

int main(void) {
    dump_mode_bytes("heat");
    return 0;
}
