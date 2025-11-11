#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pb.h>  
#include <pb_decode.h>  
#include <pb_common.h>  
#include "input.pb.h"

#define MAXLEN 128
#define PIN_EMI_REJECT_RC 86

typedef enum {
    PIN_EMI_REASON_OK = 0,
    PIN_EMI_REASON_NULL_SLICE = 1,
    PIN_EMI_REASON_LENGTH_MISMATCH = 2
} pin_emi_reason_t;

static const char *pin_emi_reason_to_string(pin_emi_reason_t reason) {
    switch (reason) {
        case PIN_EMI_REASON_OK: return "ok";
        case PIN_EMI_REASON_NULL_SLICE: return "null-pointer-with-length";
        case PIN_EMI_REASON_LENGTH_MISMATCH: return "length-field-mismatch";
        default: return "unknown";
    }
}


bool decode_json_ptr(pb_istream_t *stream, const pb_field_t *field, void **arg) {
    char *buffer = (char *)(*arg);
    size_t len = stream->bytes_left;
    if (len >= 128) {
        if (!pb_read(stream, (pb_byte_t*)buffer, 128 - 1)) {
            return false;
        }
        buffer[128 - 1] = 0;
        return true;
    }

    if (!pb_read(stream, (pb_byte_t*)buffer, len)) {
        return false;
    }
    buffer[len] = 0;
    return true;
}

bool decode_path(pb_istream_t *stream, const pb_field_t *field, void **arg) {
    char *buffer = (char *)(*arg);
    size_t len = stream->bytes_left;
    if (len >= 128) {
        if (!pb_read(stream, (pb_byte_t*)buffer, 128 - 1)) {
            return false;
        }
        buffer[128 - 1] = 0;
        return true;
    }

    if (!pb_read(stream, (pb_byte_t*)buffer, len)) {
        return false;
    }
    buffer[len] = 0;
    return true;
}

extern int mg_json_get_bool(struct mg_str json, const char * path, int * v);

// Decode from in-memory buffer and call target (for fuzzers)
int pin_wrapper_entry(const uint8_t *data, size_t len) {
    Input input = Input_init_zero;
    int emi_rc = 0;
    pin_emi_reason_t emi_reason = PIN_EMI_REASON_OK;
    const char *emi_detail = NULL;
    char json_ptr_buf[128] = {0};
    char path_buf[128] = {0};


    json_ptr_buf[0] = '\0';
    input.json.ptr.arg = json_ptr_buf;
    input.json.ptr.funcs.decode = &decode_json_ptr;

    path_buf[0] = '\0';
    input.path.arg = path_buf;
    input.path.funcs.decode = &decode_path;

    pb_istream_t stream = pb_istream_from_buffer(data, len);
    if (!pb_decode(&stream, Input_fields, &input)) {
        return 1;
    }
    int v_storage = 0;
    int * v_ptr = &v_storage;
    if (input.v.has_value) {
        v_storage = (int)(input.v.value);
    }

    mg_json_get_bool(input.json, path_buf, v_ptr);
    goto emi_finish;

emi_reject:
    emi_rc = PIN_EMI_REJECT_RC;

emi_finish:

    if (emi_rc == PIN_EMI_REJECT_RC) {
        fprintf(stderr, "[PIN_EMI] reject reason=%s%s%s\n",
                pin_emi_reason_to_string(emi_reason),
                emi_detail ? " detail=" : "",
                emi_detail ? emi_detail : "");
    }
    return emi_rc;
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
