#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <vector>

#include <google/protobuf/stubs/common.h>
#include "cpp_proto/input.pb.h"

extern "C" double calculate_distance(const struct Point * p1, const struct Point * p2);

int pin_reference_entry(const uint8_t *data, size_t len) {
    Input msg;
    if (!msg.ParseFromArray(data, static_cast<int>(len))) {
        return 1;
    }
    const auto& p1_msg = msg.p1();
    Point p1_storage = {};
    const struct Point * p1_ptr = nullptr;
    if (p1_msg.has_value()) {
        p1_storage = p1_msg.value();
        p1_ptr = &p1_storage;
    }
    const auto& p2_msg = msg.p2();
    Point p2_storage = {};
    const struct Point * p2_ptr = nullptr;
    if (p2_msg.has_value()) {
        p2_storage = p2_msg.value();
        p2_ptr = &p2_storage;
    }
    calculate_distance(p1_ptr, p2_ptr);
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
