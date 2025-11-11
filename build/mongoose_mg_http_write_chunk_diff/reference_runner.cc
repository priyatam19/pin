#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <vector>

#include <google/protobuf/stubs/common.h>
#include "cpp_proto/input.pb.h"

extern "C" void mg_http_write_chunk(struct mg_connection * c, const char * buf, size_t len);

int pin_reference_entry(const uint8_t *data, size_t len) {
    Input msg;
    if (!msg.ParseFromArray(data, static_cast<int>(len))) {
        return 1;
    }
    const auto& c_msg = msg.c();
    mg_connection c_storage = {};
    struct mg_connection * c_ptr = nullptr;
    if (c_msg.has_value()) {
        c_storage = c_msg.value();
        c_ptr = &c_storage;
    }
    mg_http_write_chunk(c_ptr, msg.buf().c_str(), msg.len());
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        std::fprintf(stderr, "Usage: %s input.bin\n", argv[0]);
        return 1;
    }

    GOOGLE_PROTOBUF_VERIFY_VERSION;

    std::ifstream ifs(argv[1], std::ios::binary);
    if (!ifs) {
        std::perror("ifstream");
        return 1;
    }
    std::vector<uint8_t> buffer((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());

    int rc = pin_reference_entry(buffer.data(), buffer.size());
    ::google::protobuf::ShutdownProtobufLibrary();
    return rc;
}
