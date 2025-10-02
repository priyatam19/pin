#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pb.h>  
#include <pb_decode.h>  
#include <pb_common.h>  
#include "input.pb.h"

#define MAXLEN 128

bool decode_primary(pb_istream_t *stream, const pb_field_t *field, void **arg) {
    char *buffer = (char *)(*arg);
    memset(buffer, 0, 128);

    pb_istream_t substream;
    if (!pb_make_string_substream(stream, &substream)) {
        return false;
    }

    size_t declared_len = substream.bytes_left;
    size_t to_read = declared_len;
    bool truncated = false;
    if (to_read >= 128) {
        truncated = true;
        to_read = 128 - 1;
    }

    bool ok = pb_read(&substream, (pb_byte_t*)buffer, to_read);
    while (ok && substream.bytes_left > 0) {
        uint8_t scratch[32];
        size_t chunk = substream.bytes_left < sizeof(scratch) ? substream.bytes_left : sizeof(scratch);
        ok = pb_read(&substream, scratch, chunk);
    }

    if (ok && truncated) {
        ok = false;
    }

    if (ok && memchr(buffer, 0, to_read) != NULL) {
        ok = false;
    }

    if (ok) {
#ifdef PB_VALIDATE_UTF8
        ok = pb_validate_utf8(buffer);
#else
        const unsigned char *p = (const unsigned char *)buffer;
        while (ok && *p) {
            unsigned char c = *p++;
            if (c < 0x80) {
                continue;
            }
            ok = false;
        }
#endif
    }

    pb_close_string_substream(stream, &substream);
    return ok;
}

bool decode_secondary(pb_istream_t *stream, const pb_field_t *field, void **arg) {
    char *buffer = (char *)(*arg);
    memset(buffer, 0, 128);

    pb_istream_t substream;
    if (!pb_make_string_substream(stream, &substream)) {
        return false;
    }

    size_t declared_len = substream.bytes_left;
    size_t to_read = declared_len;
    bool truncated = false;
    if (to_read >= 128) {
        truncated = true;
        to_read = 128 - 1;
    }

    bool ok = pb_read(&substream, (pb_byte_t*)buffer, to_read);
    while (ok && substream.bytes_left > 0) {
        uint8_t scratch[32];
        size_t chunk = substream.bytes_left < sizeof(scratch) ? substream.bytes_left : sizeof(scratch);
        ok = pb_read(&substream, scratch, chunk);
    }

    if (ok && truncated) {
        ok = false;
    }

    if (ok && memchr(buffer, 0, to_read) != NULL) {
        ok = false;
    }

    if (ok) {
#ifdef PB_VALIDATE_UTF8
        ok = pb_validate_utf8(buffer);
#else
        const unsigned char *p = (const unsigned char *)buffer;
        while (ok && *p) {
            unsigned char c = *p++;
            if (c < 0x80) {
                continue;
            }
            ok = false;
        }
#endif
    }

    pb_close_string_substream(stream, &substream);
    return ok;
}

extern int compare_modes(const char * primary, const char * secondary);

// Decode from in-memory buffer and call target (for fuzzers)
int pin_wrapper_entry(const uint8_t *data, size_t len) {
    Input input = Input_init_zero;
    char primary_buf[128];
    char secondary_buf[128];


    primary_buf[0] = '\0';
    input.primary.arg = primary_buf;
    input.primary.funcs.decode = &decode_primary;

    secondary_buf[0] = '\0';
    input.secondary.arg = secondary_buf;
    input.secondary.funcs.decode = &decode_secondary;

    pb_istream_t stream = pb_istream_from_buffer(data, len);
    if (!pb_decode(&stream, Input_fields, &input)) {
        return 1;
    }
    compare_modes(primary_buf, secondary_buf);
    return 0;
}

#ifndef PIN_WRAPPER_NO_MAIN
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

    int rc = pin_wrapper_entry(buf, (size_t)len);
    free(buf);
    return rc;
}
#endif
