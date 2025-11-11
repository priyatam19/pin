#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "mongoose.h"

#include "input.pb.h"
#include "pb_encode.h"
#include "pb_common.h"

#ifndef MQTT_VERSION
#define MQTT_VERSION 4
#endif

struct string_arg {
    const uint8_t *data;
    size_t len;
};

static bool encode_string_cb(pb_ostream_t *stream, const pb_field_t *field, void * const *arg) {
    const struct string_arg *sa = (const struct string_arg *)(*arg);
    if (sa == NULL || sa->data == NULL) {
        if (!pb_encode_tag_for_field(stream, field)) return false;
        return pb_encode_string(stream, NULL, 0);
    }
    if (!pb_encode_tag_for_field(stream, field)) return false;
    return pb_encode_string(stream, sa->data, (size_t)sa->len);
}

static void clear_mg_str(mg_str *dst) {
    dst->ptr.funcs.encode = NULL;
    dst->ptr.arg = NULL;
    dst->len = 0;
}

static void fill_mg_str(mg_str *dst, struct string_arg *arg, const struct mg_str *src) {
    if (src && src->ptr && src->len > 0) {
        arg->data = (const uint8_t *)src->ptr;
        arg->len = src->len;
        dst->ptr.funcs.encode = encode_string_cb;
        dst->ptr.arg = arg;
        dst->len = (int32_t)src->len;
    } else {
        clear_mg_str(dst);
        if (arg) {
            arg->data = NULL;
            arg->len = 0;
        }
    }
}

static void fill_mg_str_ptr(MgStrPtr *dst, struct string_arg *arg, const struct mg_str *src) {
    if (src && src->ptr && src->len > 0) {
        dst->present = true;
        dst->has_value = true;
        fill_mg_str(&dst->value, arg, src);
    } else {
        dst->present = false;
        dst->has_value = false;
        clear_mg_str(&dst->value);
        if (arg) {
            arg->data = NULL;
            arg->len = 0;
        }
    }
}

static int convert_file(const char *in_path, const char *out_path) {
    int rc = 1;
    uint8_t *buf = NULL;
    FILE *fp = fopen(in_path, "rb");
    if (!fp) {
        perror("fopen");
        return 2;
    }
    fseek(fp, 0, SEEK_END);
    long fsize = ftell(fp);
    rewind(fp);
    if (fsize <= 0 || fsize > 65535) {
        fprintf(stderr, "[!] Unexpected input size %ld\n", fsize);
        fclose(fp);
        return 3;
    }
    buf = (uint8_t *)malloc((size_t)fsize);
    if (!buf) {
        perror("malloc");
        fclose(fp);
        return 4;
    }
    if (fread(buf, 1, (size_t)fsize, fp) != (size_t)fsize) {
        perror("fread");
        goto cleanup;
    }
    fclose(fp);
    fp = NULL;

    struct mg_mqtt_message mm;
    memset(&mm, 0, sizeof(mm));

    int parse_rc = mg_mqtt_parse(buf, (size_t)fsize, MQTT_VERSION, &mm);
    if (parse_rc != MQTT_OK) {
        fprintf(stderr, "[!] mg_mqtt_parse failed (%d) for %s\n", parse_rc, in_path);
        goto cleanup;
    }

    Input input = Input_init_zero;
    input.has_msg = true;
    input.msg.present = true;
    input.msg.has_value = true;

    struct string_arg topic_arg = {0};
    struct string_arg data_arg = {0};
    struct string_arg dgram_arg = {0};

    mg_mqtt_message *out = &input.msg.value;

    fill_mg_str_ptr(&out->topic, &topic_arg, &mm.topic);
    out->has_topic = out->topic.present;

    out->has_data = true;
    fill_mg_str(&out->data, &data_arg, &mm.data);

    out->has_dgram = true;
    fill_mg_str(&out->dgram, &dgram_arg, &mm.dgram);

    out->id = mm.id;
    out->cmd = mm.cmd;
    out->ack = mm.ack;
    out->props_start = mm.props_start;
    out->props_size = mm.props_size;

    out->has_qos = true;
    out->qos.has_value = true;
    out->qos.value = mm.qos;

    input.pos = 4; /* Initial offset after message ID */

    size_t encoded_size = 0;
    if (!pb_get_encoded_size(&encoded_size, Input_fields, &input)) {
        fprintf(stderr, "[!] Failed to compute encoded size\n");
        goto cleanup;
    }

    uint8_t *encoded = (uint8_t *)malloc(encoded_size);
    if (!encoded) {
        perror("malloc encoded");
        goto cleanup;
    }

    pb_ostream_t ostream = pb_ostream_from_buffer(encoded, encoded_size);
    if (!pb_encode(&ostream, Input_fields, &input)) {
        fprintf(stderr, "[!] pb_encode failed: %s\n", PB_GET_ERROR(&ostream));
        free(encoded);
        goto cleanup;
    }

    FILE *outf = fopen(out_path, "wb");
    if (!outf) {
        perror("fopen output");
        free(encoded);
        goto cleanup;
    }
    if (fwrite(encoded, 1, ostream.bytes_written, outf) != ostream.bytes_written) {
        perror("fwrite output");
        fclose(outf);
        free(encoded);
        goto cleanup;
    }
    fclose(outf);
    free(encoded);

    rc = 0;

cleanup:
    if (buf) free(buf);
    if (fp) fclose(fp);
    return rc;
}

int main(int argc, char **argv) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <afl_crash.bin> <out.pb>\n", argv[0]);
        return 1;
    }
    return convert_file(argv[1], argv[2]);
}
