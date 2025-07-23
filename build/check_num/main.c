#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pb.h>  
#include <pb_decode.h>  
#include "input.pb.h"

#define MAXLEN 128

extern int main();

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


    pb_istream_t stream = pb_istream_from_buffer(buf, len);
    if (!pb_decode(&stream, Input_fields, &input)) {
        fprintf(stderr, "pb_decode failed: %s\n", PB_GET_ERROR(&stream));
        free(buf);
        return 1;
    }
    free(buf);

    int result = original_main();
    printf("Output: %d\n", result);
    return result;
}
