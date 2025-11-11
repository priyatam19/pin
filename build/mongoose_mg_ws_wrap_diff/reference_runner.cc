#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <vector>

#include <google/protobuf/stubs/common.h>
#include "cpp_proto/input.pb.h"

extern "C" int mg_ws_wrap(struct mg_connection * c, size_t len, int op);

int pin_reference_entry(const uint8_t *data, size_t len) {
    Input msg;
    if (!msg.ParseFromArray(data, static_cast<int>(len))) {
        return 1;
    }
    std::vector<mg_connection> c_vec;
    struct mg_connection * c_ptr = nullptr;
    size_t c_len = 0;
    if (msg.has_c()) {
        const auto& c_msg = msg.c();
        c_len = static_cast<size_t>(c_msg.data_size());
        c_vec.reserve(c_len);
        for (int i = 0; i < c_msg.data_size(); ++i) {
            c_vec.push_back(c_msg.data(i));
        }
        if (!c_vec.empty()) {
            c_ptr = (struct mg_connection *)c_vec.data();
        }
    }
    mg_ws_wrap(c_ptr, (size_t)c_len, msg.op());
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
