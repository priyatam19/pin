#include "raw_passthrough_example.h"

#include <stdio.h>

int raw_passthrough_parser(const uint8_t *buf, size_t len, uint8_t version,
                           struct raw_passthrough_ctx *ctx) {
    if (ctx == NULL || buf == NULL) {
        return -1;
    }
    ctx->last_len = len;
    ctx->version = version;

    /* Treat a leading 0x82 (MQTT publish) as interesting so the fuzzer has a goal. */
    if (len > 0 && buf[0] == 0x82) {
        return 1;
    }
    return 0;
}
