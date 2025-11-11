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

typedef struct MgQueuePtr_DecodeCtx {
    mg_queue *data;
    size_t count;
    size_t capacity;
} MgQueuePtr_DecodeCtx;

static bool decode_MgQueuePtr_data(pb_istream_t *stream, const pb_field_t *field, void **arg) {
    (void)field;
    MgQueuePtr_DecodeCtx *ctx = (MgQueuePtr_DecodeCtx*)(*arg);
    while (stream->bytes_left > 0) {
        (void)stream;
        (void)field;
        return false;
        if (ctx->count >= ctx->capacity) {
            size_t new_capacity = ctx->capacity ? ctx->capacity * 2 : 8;
            mg_queue *new_data = (mg_queue*)realloc(ctx->data, new_capacity * sizeof(mg_queue));
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


extern void mg_queue_del(struct mg_queue * q, size_t len);

// Decode from in-memory buffer and call target (for fuzzers)
int pin_wrapper_entry(const uint8_t *data, size_t len) {
    Input input = Input_init_zero;
    int emi_rc = 0;
    pin_emi_reason_t emi_reason = PIN_EMI_REASON_OK;
    const char *emi_detail = NULL;

    MgQueuePtr_DecodeCtx q_ctx = {NULL, 0, 0};
    input.q.data.funcs.decode = decode_MgQueuePtr_data;
    input.q.data.arg = &q_ctx;

    pb_istream_t stream = pb_istream_from_buffer(data, len);
    if (!pb_decode(&stream, Input_fields, &input)) {
        if (q_ctx.data) { free(q_ctx.data); }
        return 1;
    }
    uint64_t q_length_raw = input.q.length;
    size_t q_len = q_ctx.count;
    mg_queue * q_storage = q_ctx.data;
    q_ctx.data = NULL;
    struct mg_queue * q_ptr = NULL;
    if (q_storage && q_len > 0) {
        q_ptr = (struct mg_queue *)q_storage;
    }
    input.has_q = (q_len > 0);
    input.q.length = (uint64_t)q_len;

    if (q_len > 0 && q_ptr == NULL) {
        emi_reason = PIN_EMI_REASON_NULL_SLICE;
        emi_detail = "q";
        goto emi_reject;
    }
    if ((uint64_t)q_len != q_length_raw) {
        emi_reason = PIN_EMI_REASON_LENGTH_MISMATCH;
        emi_detail = "q";
        goto emi_reject;
    }
    if (input.len != (size_t)q_len) {
        emi_reason = PIN_EMI_REASON_LENGTH_MISMATCH;
        emi_detail = "len";
        goto emi_reject;
    }
    mg_queue_del(q_ptr, (size_t)q_len);
    goto emi_finish;

emi_reject:
    emi_rc = PIN_EMI_REJECT_RC;

emi_finish:
    if (q_storage) { free(q_storage); }

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
