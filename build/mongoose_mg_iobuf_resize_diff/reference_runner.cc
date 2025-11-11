#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <vector>

#include <google/protobuf/stubs/common.h>
#include "cpp_proto/input.pb.h"

extern "C" int mg_iobuf_resize(struct mg_iobuf * io, size_t new_size);

int pin_reference_entry(const uint8_t *data, size_t len) {
    Input msg;
    if (!msg.ParseFromArray(data, static_cast<int>(len))) {
        return 1;
    }
    std::vector<mg_iobuf> io_vec;
    struct mg_iobuf * io_ptr = nullptr;
    size_t io_len = 0;
    if (msg.has_io()) {
        const auto& io_msg = msg.io();
        io_len = static_cast<size_t>(io_msg.data_size());
        io_vec.reserve(io_len);
        for (int i = 0; i < io_msg.data_size(); ++i) {
            io_vec.push_back(io_msg.data(i));
        }
        if (!io_vec.empty()) {
            io_ptr = (struct mg_iobuf *)io_vec.data();
        }
    }
    mg_iobuf_resize(io_ptr, (size_t)io_len);
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
