#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <pb.h>
#include <pb_decode.h>
#include <pb_common.h>
#include "input.pb.h"

struct mg_connection; /* forward declare */

extern void mg_mqtt_send_header(struct mg_connection *c, uint8_t cmd, uint8_t flags, uint32_t len);

int pin_wrapper_entry(const uint8_t *data, size_t size) {
    Input input = Input_init_zero;
    pb_istream_t stream = pb_istream_from_buffer(data, size);
    if (!pb_decode(&stream, Input_fields, &input)) {
        return 1;
    }

    uint8_t cmd = input.cmd > 0xffu ? (uint8_t)(input.cmd & 0xffu) : (uint8_t)input.cmd;
    uint8_t flags = input.flags > 0xffu ? (uint8_t)(input.flags & 0xffu) : (uint8_t)input.flags;
    uint32_t len = input.len;

    mg_mqtt_send_header(NULL, cmd, flags, len);
    return 0;
}

#ifndef PIN_WRAPPER_NO_MAIN
int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s input.bin\n", argv[0]);
        return 1;
    }

    FILE *f = fopen(argv[1], "rb");
    if (!f) {
        perror("fopen");
        return 1;
    }
    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    rewind(f);

    if (len < 0) {
        perror("ftell");
        fclose(f);
        return 1;
    }

    uint8_t *buf = (uint8_t *)malloc((size_t)len);
    if (!buf) {
        perror("malloc");
        fclose(f);
        return 1;
    }

    size_t read = fread(buf, 1, (size_t)len, f);
    fclose(f);
    if (read != (size_t)len) {
        perror("fread");
        free(buf);
        return 1;
    }

    int rc = pin_wrapper_entry(buf, (size_t)len);
    free(buf);
    return rc;
}
#endif
