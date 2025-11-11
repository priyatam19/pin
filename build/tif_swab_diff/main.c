#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pb.h>  
#include <pb_decode.h>  
#include <pb_common.h>  
#include "input.pb.h"
#include "/home/priyatam/libtiff/libtiff/tiffio.h"

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

typedef struct Uint32Slice_DecodeCtx {
    uint8 *data;
    size_t count;
    size_t capacity;
} Uint32Slice_DecodeCtx;

static bool decode_Uint32Slice_data(pb_istream_t *stream, const pb_field_t *field, void **arg) {
    (void)field;
    Uint32Slice_DecodeCtx *ctx = (Uint32Slice_DecodeCtx*)(*arg);
    while (stream->bytes_left > 0) {
        uint64_t tmp = 0;
        if (!pb_decode_varint(stream, &tmp)) {
            return false;
        }
        uint8 value = (uint8)((uint32_t)tmp);
        if (ctx->count >= ctx->capacity) {
            size_t new_capacity = ctx->capacity ? ctx->capacity * 2 : 8;
            uint8 *new_data = (uint8*)realloc(ctx->data, new_capacity * sizeof(uint8));
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


extern void TIFFReverseBits(uint8 * cp, tmsize_t n);

// Decode from in-memory buffer and call target (for fuzzers)
int pin_wrapper_entry(const uint8_t *data, size_t len) {
    Input input = Input_init_zero;
    int emi_rc = 0;
    pin_emi_reason_t emi_reason = PIN_EMI_REASON_OK;
    const char *emi_detail = NULL;

    Uint32Slice_DecodeCtx cp_ctx = {NULL, 0, 0};
    input.cp.data.funcs.decode = decode_Uint32Slice_data;
    input.cp.data.arg = &cp_ctx;

    pb_istream_t stream = pb_istream_from_buffer(data, len);
    if (!pb_decode(&stream, Input_fields, &input)) {
        if (cp_ctx.data) { free(cp_ctx.data); }
        return 1;
    }
    int64_t cp_length_raw = input.cp.length;
    size_t cp_len = cp_ctx.count;
    uint8 * cp_storage = cp_ctx.data;
    cp_ctx.data = NULL;
    uint8 * cp_ptr = NULL;
    if (cp_storage && cp_len > 0) {
        cp_ptr = (uint8 *)cp_storage;
    }
    input.has_cp = (cp_len > 0);
    input.cp.length = (int64_t)cp_len;

    if (cp_len > 0 && cp_ptr == NULL) {
        emi_reason = PIN_EMI_REASON_NULL_SLICE;
        emi_detail = "cp";
        goto emi_reject;
    }
    if ((int64_t)cp_len != cp_length_raw) {
        emi_reason = PIN_EMI_REASON_LENGTH_MISMATCH;
        emi_detail = "cp";
        goto emi_reject;
    }
    if (input.n != (tmsize_t)cp_len) {
        emi_reason = PIN_EMI_REASON_LENGTH_MISMATCH;
        emi_detail = "n";
        goto emi_reject;
    }
    TIFFReverseBits(cp_ptr, (tmsize_t)cp_len);
    goto emi_finish;

emi_reject:
    emi_rc = PIN_EMI_REJECT_RC;

emi_finish:
    if (cp_storage) { free(cp_storage); }

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
