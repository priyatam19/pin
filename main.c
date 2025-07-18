#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "input.pb.h"
#include "pb_decode.h"

#define MAXLEN 128

bool decode_s(pb_istream_t *stream, const pb_field_t *field, void **arg) {
    char *buffer = (char *)(*arg);
    size_t len = stream->bytes_left < (4096 - 1) ? stream->bytes_left : (4096 - 1);
    if (!pb_read(stream, (pb_byte_t*)buffer, len)) return false;
    buffer[len] = '\0';
    return true;
}

// User logic function
extern int P(const Input* input, const char* s_buf);

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s input.bin\n", argv[0]);
        return 1;
    }

    FILE *f = fopen(argv[1], "rb");
    if (!f) { perror("fopen"); return 1; }
    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    rewind(f);

    uint8_t *buf = malloc(len);
    if (!buf) { perror("malloc"); fclose(f); return 1; }
    fread(buf, 1, len, f);
    fclose(f);

    Input input = Input_init_zero;
    char s_buf[4096];


    s_buf[0] = '\0';
    input.s.arg = s_buf;
    input.s.funcs.decode = &decode_s;

    pb_istream_t stream = pb_istream_from_buffer(buf, len);
    if (!pb_decode(&stream, Input_fields, &input)) {
        fprintf(stderr, "pb_decode failed\n");
        free(buf);
        return 1;
    }
    free(buf);

    int result = P(&input, s_buf);
    printf("Output: %d\n", result);
    return 0;
}
