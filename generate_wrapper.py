import sys

# ---- Template for the C wrapper ----
WRAPPER_TEMPLATE = r'''
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "a.pb.h"
#include "pb_decode.h"

#define MAX_MESSAGE_LEN 128

// Nanopb callback: read string field to buffer
bool read_string_callback(pb_istream_t *stream, const pb_field_t *field, void **arg) {{
    char *buffer = (char *)(*arg);
    size_t len = stream->bytes_left < (MAX_MESSAGE_LEN - 1) ? stream->bytes_left : (MAX_MESSAGE_LEN - 1);
    if (!pb_read(stream, (pb_byte_t*)buffer, len)) return false;
    buffer[len] = '\0';  // Null-terminate
    return true;
}}

// User logic function
extern int {func_name}(const {struct_name}* input, const char* {msgbuf_name});

int main(int argc, char *argv[]) {{
    if (argc != 2) {{
        fprintf(stderr, "Usage: %s input.bin\n", argv[0]);
        return 1;
    }}

    FILE *f = fopen(argv[1], "rb");
    if (!f) {{ perror("fopen"); return 1; }}
    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    rewind(f);

    uint8_t *buf = malloc(len);
    if (!buf) {{ perror("malloc"); fclose(f); return 1; }}
    fread(buf, 1, len, f);
    fclose(f);

    {struct_name} input = {struct_name}_init_zero;
    char {msgbuf_name}[MAX_MESSAGE_LEN] = {{0}};
    input.message.arg = {msgbuf_name};
    input.message.funcs.decode = &read_string_callback;

    pb_istream_t stream = pb_istream_from_buffer(buf, len);
    if (!pb_decode(&stream, {struct_name}_fields, &input)) {{
        fprintf(stderr, "pb_decode failed\n");
        free(buf);
        return 1;
    }}
    free(buf);

    int result = {func_name}(&input, {msgbuf_name});
    printf("Output: %d\n", result);
    return 0;
}}
'''

def generate_wrapper(struct_name, func_name, msgbuf_name, output_file="main.c"):
    code = WRAPPER_TEMPLATE.format(
        struct_name=struct_name,
        func_name=func_name,
        msgbuf_name=msgbuf_name
    )
    with open(output_file, "w") as f:
        f.write(code)
    print(f"Wrapper written to {output_file}")

if __name__ == "__main__":
    # Usage: python generate_wrapper.py <StructName> <FuncName> <MsgBufName>
    # Example: python generate_wrapper.py A P message_buf
    if len(sys.argv) != 4:
        print("Usage: python generate_wrapper.py <StructName> <FuncName> <MsgBufName>")
        sys.exit(1)
    struct_name = sys.argv[1]
    func_name = sys.argv[2]
    msgbuf_name = sys.argv[3]
    generate_wrapper(struct_name, func_name, msgbuf_name)