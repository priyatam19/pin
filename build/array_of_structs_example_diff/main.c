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

typedef struct SamplePtr_DecodeCtx {
    Sample *data;
    size_t count;
    size_t capacity;
} SamplePtr_DecodeCtx;

static bool decode_SamplePtr_data(pb_istream_t *stream, const pb_field_t *field, void **arg) {
    (void)field;
    SamplePtr_DecodeCtx *ctx = (SamplePtr_DecodeCtx*)(*arg);
    while (stream->bytes_left > 0) {
        (void)stream;
        (void)field;
        return false;
        if (ctx->count >= ctx->capacity) {
            size_t new_capacity = ctx->capacity ? ctx->capacity * 2 : 8;
            Sample *new_data = (Sample*)realloc(ctx->data, new_capacity * sizeof(Sample));
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


extern float average_samples(const struct Sample * samples, int count);

// Decode from in-memory buffer and call target (for fuzzers)
int pin_wrapper_entry(const uint8_t *data, size_t len) {
    Input input = Input_init_zero;
    int emi_rc = 0;
    pin_emi_reason_t emi_reason = PIN_EMI_REASON_OK;
    const char *emi_detail = NULL;

    SamplePtr_DecodeCtx samples_ctx = {NULL, 0, 0};
    input.samples.data.funcs.decode = decode_SamplePtr_data;
    input.samples.data.arg = &samples_ctx;

    pb_istream_t stream = pb_istream_from_buffer(data, len);
    if (!pb_decode(&stream, Input_fields, &input)) {
        if (samples_ctx.data) { free(samples_ctx.data); }
        return 1;
    }
    int32_t samples_length_raw = input.samples.length;
    size_t samples_len = samples_ctx.count;
    Sample * samples_storage = samples_ctx.data;
    samples_ctx.data = NULL;
    const struct Sample * samples_ptr = NULL;
    if (samples_storage && samples_len > 0) {
        samples_ptr = (const struct Sample *)samples_storage;
    }
    input.has_samples = (samples_len > 0);
    input.samples.length = (int32_t)samples_len;

    if (samples_len > 0 && samples_ptr == NULL) {
        emi_reason = PIN_EMI_REASON_NULL_SLICE;
        emi_detail = "samples";
        goto emi_reject;
    }
    if ((int32_t)samples_len != samples_length_raw) {
        emi_reason = PIN_EMI_REASON_LENGTH_MISMATCH;
        emi_detail = "samples";
        goto emi_reject;
    }
    if (input.count != (int)samples_len) {
        emi_reason = PIN_EMI_REASON_LENGTH_MISMATCH;
        emi_detail = "count";
        goto emi_reject;
    }
    average_samples(samples_ptr, (int)samples_len);
    goto emi_finish;

emi_reject:
    emi_rc = PIN_EMI_REJECT_RC;

emi_finish:
    if (samples_storage) { free(samples_storage); }

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
