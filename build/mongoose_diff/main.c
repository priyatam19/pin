#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mongoose.h"

#define PIN_EMI_REJECT_RC 86



int pin_wrapper_entry(const uint8_t *data, size_t len) {
    (void)data;
    (void)len;
    uint8_t pin_val_arg2 = 0;
    struct mg_mqtt_message pin_ptr_arg3_storage;
    memset(&pin_ptr_arg3_storage, 0, sizeof(pin_ptr_arg3_storage));
    struct mg_mqtt_message * pin_ptr_arg3 = (struct mg_mqtt_message *)&pin_ptr_arg3_storage;
    mg_mqtt_parse((const uint8_t *)data, (size_t)len, pin_val_arg2, pin_ptr_arg3);
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
    long file_len = ftell(f);
    rewind(f);

    if (file_len < 0) {
        perror("ftell");
        fclose(f);
        return 1;
    }

    uint8_t *buf = malloc((size_t)file_len);
    if (!buf) { perror("malloc"); fclose(f); return 1; }
    size_t read_len = fread(buf, 1, (size_t)file_len, f);
    fclose(f);
    if (read_len != (size_t)file_len) {
        perror("fread");
        free(buf);
        return 1;
    }

    int rc = pin_wrapper_entry(buf, (size_t)file_len);
    free(buf);
    return rc;
}
#endif
