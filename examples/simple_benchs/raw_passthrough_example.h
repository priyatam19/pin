#pragma once

#include <stddef.h>
#include <stdint.h>

struct raw_passthrough_ctx {
    size_t last_len;
    uint8_t version;
};

int raw_passthrough_parser(const uint8_t *buf, size_t len, uint8_t version,
                           struct raw_passthrough_ctx *ctx);
