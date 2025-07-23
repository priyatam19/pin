#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pb.h>  
#include <pb_decode.h>  
#include "mystruct.pb.h"

#define MAXLEN 128

bool decode_name(pb_istream_t *stream, const pb_field_t *field, void **arg) {
    char *buffer = (char *)(*arg);
    size_t len = stream->bytes_left < (32 - 1) ? stream->bytes_left : (32 - 1);
    if (!pb_read(stream, (pb_byte_t*)buffer, len)) return false;
    buffer[len] = '\0';
    return true;
}

bool decode_sub_desc(pb_istream_t *stream, const pb_field_t *field, void **arg) {
    char *buffer = (char *)(*arg);
    size_t len = stream->bytes_left < (16 - 1) ? stream->bytes_left : (16 - 1);
    if (!pb_read(stream, (pb_byte_t*)buffer, len)) return false;
    buffer[len] = '\0';
    return true;
}

extern int P(const MyStruct* input, const char* name_buf, const char* sub_desc_buf);

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

    MyStruct input = MyStruct_init_zero;
    char name_buf[32];
    char sub_desc_buf[16];


    name_buf[0] = '\0';
    input.name.arg = name_buf;
    input.name.funcs.decode = &decode_name;

    sub_desc_buf[0] = '\0';
    input.sub.desc.arg = sub_desc_buf;
    input.sub.desc.funcs.decode = &decode_sub_desc;

    pb_istream_t stream = pb_istream_from_buffer(buf, len);
    if (!pb_decode(&stream, MyStruct_fields, &input)) {
        fprintf(stderr, "pb_decode failed: %s\n", PB_GET_ERROR(&stream));
        free(buf);
        return 1;
    }
    free(buf);

    int result = P(&input, name_buf, sub_desc_buf);
    printf("Output: %d\n", result);
    return result;
}
