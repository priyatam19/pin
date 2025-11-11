#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <vector>

#include <google/protobuf/stubs/common.h>
#include "cpp_proto/input.pb.h"
#include "/home/priyatam/libtiff/libtiff/tiffio.h"

extern "C" void TIFFReverseBits(uint8 * cp, tmsize_t n);

int pin_reference_entry(const uint8_t *data, size_t len) {
    Input msg;
    if (!msg.ParseFromArray(data, static_cast<int>(len))) {
        return 1;
    }
    std::vector<uint8> cp_vec;
    uint8 * cp_ptr = nullptr;
    size_t cp_len = 0;
    if (msg.has_cp()) {
        const auto& cp_msg = msg.cp();
        cp_len = static_cast<size_t>(cp_msg.data_size());
        cp_vec.reserve(cp_len);
        for (int i = 0; i < cp_msg.data_size(); ++i) {
            cp_vec.push_back(cp_msg.data(i));
        }
        if (!cp_vec.empty()) {
            cp_ptr = (uint8 *)cp_vec.data();
        }
    }
    TIFFReverseBits(cp_ptr, (tmsize_t)cp_len);
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
