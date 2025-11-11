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

typedef struct Uint32Slice_DecodeCtx {
    uint8_t *data;
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
        uint8_t value = (uint8_t)((uint32_t)tmp);
        if (ctx->count >= ctx->capacity) {
            size_t new_capacity = ctx->capacity ? ctx->capacity * 2 : 8;
            uint8_t *new_data = (uint8_t*)realloc(ctx->data, new_capacity * sizeof(uint8_t));
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


extern int mg_mqtt_parse(const uint8_t * buf, size_t len, uint8_t version, struct mg_mqtt_message * m);

// Decode from in-memory buffer and call target (for fuzzers)
int pin_wrapper_entry(const uint8_t *data, size_t len) {
    Input input = Input_init_zero;
    int emi_rc = 0;
    pin_emi_reason_t emi_reason = PIN_EMI_REASON_OK;
    const char *emi_detail = NULL;

    Uint32Slice_DecodeCtx buf_ctx = {NULL, 0, 0};
    input.buf.data.funcs.decode = decode_Uint32Slice_data;
    input.buf.data.arg = &buf_ctx;

    pb_istream_t stream = pb_istream_from_buffer(data, len);
    if (!pb_decode(&stream, Input_fields, &input)) {
        if (buf_ctx.data) { free(buf_ctx.data); }
        return 1;
    }
    uint64_t buf_length_raw = input.buf.length;
    size_t buf_len = buf_ctx.count;
    uint8_t * buf_storage = buf_ctx.data;
    buf_ctx.data = NULL;
    const uint8_t * buf_ptr = NULL;
    if (buf_storage && buf_len > 0) {
        buf_ptr = (const uint8_t *)buf_storage;
    }
    input.has_buf = (buf_len > 0);
    input.buf.length = (uint64_t)buf_len;
    mg_mqtt_message m_storage = {0};
    struct mg_mqtt_message * m_ptr = NULL;
    if (input.m.present) {
        m_storage = input.m.value;
        m_ptr = (struct mg_mqtt_message *)&m_storage;
    }

    if (buf_len > 0 && buf_ptr == NULL) {
        emi_reason = PIN_EMI_REASON_NULL_SLICE;
        emi_detail = "buf";
        goto emi_reject;
    }
    if ((uint64_t)buf_len != buf_length_raw) {
        emi_reason = PIN_EMI_REASON_LENGTH_MISMATCH;
        emi_detail = "buf";
        goto emi_reject;
    }
    if (input.len != (size_t)buf_len) {
        emi_reason = PIN_EMI_REASON_LENGTH_MISMATCH;
        emi_detail = "len";
        goto emi_reject;
    }
    mg_mqtt_parse(buf_ptr, (size_t)buf_len, input.version, m_ptr);
    goto emi_finish;

emi_reject:
    emi_rc = PIN_EMI_REJECT_RC;

emi_finish:
    if (buf_storage) { free(buf_storage); }

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
