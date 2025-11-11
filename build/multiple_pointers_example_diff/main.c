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

typedef struct FloatSlice_DecodeCtx {
    float *data;
    size_t count;
    size_t capacity;
} FloatSlice_DecodeCtx;

static bool decode_FloatSlice_data(pb_istream_t *stream, const pb_field_t *field, void **arg) {
    (void)field;
    FloatSlice_DecodeCtx *ctx = (FloatSlice_DecodeCtx*)(*arg);
    while (stream->bytes_left > 0) {
        uint32_t raw = 0;
        if (!pb_decode_fixed32(stream, &raw)) {
            return false;
        }
        union { uint32_t u; float f; } conv;
        conv.u = raw;
        float value = (float)conv.f;
        if (ctx->count >= ctx->capacity) {
            size_t new_capacity = ctx->capacity ? ctx->capacity * 2 : 8;
            float *new_data = (float*)realloc(ctx->data, new_capacity * sizeof(float));
            if (!new_data) {
                return false;
            }
            ctx->data = new_data;
            ctx->capacity = new_capacity;
        }
        ctx->data[ctx->count++] = value;
    }
    return true;
}


bool decode_label(pb_istream_t *stream, const pb_field_t *field, void **arg) {
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

extern int process_data(int * error_code, const float * readings, int num_readings, const char * label);

// Decode from in-memory buffer and call target (for fuzzers)
int pin_wrapper_entry(const uint8_t *data, size_t len) {
    Input input = Input_init_zero;
    int emi_rc = 0;
    pin_emi_reason_t emi_reason = PIN_EMI_REASON_OK;
    const char *emi_detail = NULL;
    char label_buf[128] = {0};

    FloatSlice_DecodeCtx readings_ctx = {NULL, 0, 0};
    input.readings.data.funcs.decode = decode_FloatSlice_data;
    input.readings.data.arg = &readings_ctx;

    label_buf[0] = '\0';
    input.label.arg = label_buf;
    input.label.funcs.decode = &decode_label;

    pb_istream_t stream = pb_istream_from_buffer(data, len);
    if (!pb_decode(&stream, Input_fields, &input)) {
        if (readings_ctx.data) { free(readings_ctx.data); }
        return 1;
    }
    int error_code_storage = 0;
    int * error_code_ptr = NULL;
    if (input.error_code.has_value) {
        error_code_storage = (int)(input.error_code.value);
        error_code_ptr = &error_code_storage;
    }
    int32_t readings_length_raw = input.readings.length;
    size_t readings_len = readings_ctx.count;
    float * readings_storage = readings_ctx.data;
    readings_ctx.data = NULL;
    const float * readings_ptr = NULL;
    if (readings_storage && readings_len > 0) {
        readings_ptr = (const float *)readings_storage;
    }
    input.has_readings = (readings_len > 0);
    input.readings.length = (int32_t)readings_len;

    if (readings_len > 0 && readings_ptr == NULL) {
        emi_reason = PIN_EMI_REASON_NULL_SLICE;
        emi_detail = "readings";
        goto emi_reject;
    }
    if ((int32_t)readings_len != readings_length_raw) {
        emi_reason = PIN_EMI_REASON_LENGTH_MISMATCH;
        emi_detail = "readings";
        goto emi_reject;
    }
    if (input.num_readings != (int)readings_len) {
        emi_reason = PIN_EMI_REASON_LENGTH_MISMATCH;
        emi_detail = "num_readings";
        goto emi_reject;
    }
    process_data(error_code_ptr, readings_ptr, (int)readings_len, label_buf);
    goto emi_finish;

emi_reject:
    emi_rc = PIN_EMI_REJECT_RC;

emi_finish:
    if (readings_storage) { free(readings_storage); }

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
