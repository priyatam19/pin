#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <vector>

#include <google/protobuf/stubs/common.h>
#include "cpp_proto/input.pb.h"

extern "C" int sum_slice(const int * values, int count);

int pin_reference_entry(const uint8_t *data, size_t len) {
    Input msg;
    if (!msg.ParseFromArray(data, static_cast<int>(len))) {
        return 1;
    }
    std::vector<int> values_vec;
    const int * values_ptr = nullptr;
    size_t values_len = 0;
    if (msg.has_values()) {
        const auto& values_msg = msg.values();
        values_len = static_cast<size_t>(values_msg.data_size());
        values_vec.reserve(values_len);
        for (int i = 0; i < values_msg.data_size(); ++i) {
            values_vec.push_back(values_msg.data(i));
        }
        if (!values_vec.empty()) {
            values_ptr = (const int *)values_vec.data();
        }
    }
    sum_slice(values_ptr, (int)values_len);
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
