#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <vector>

#include <google/protobuf/stubs/common.h>
#include "cpp_proto/input.pb.h"

extern "C" int process_data(int * error_code, const float * readings, int num_readings, const char * label);

int pin_reference_entry(const uint8_t *data, size_t len) {
    Input msg;
    if (!msg.ParseFromArray(data, static_cast<int>(len))) {
        return 1;
    }
    const auto& error_code_msg = msg.error_code();
    int error_code_storage = 0;
    int * error_code_ptr = nullptr;
    if (error_code_msg.has_value()) {
        error_code_storage = static_cast<int>(error_code_msg.value());
        error_code_ptr = &error_code_storage;
    }
    std::vector<float> readings_vec;
    const float * readings_ptr = nullptr;
    size_t readings_len = 0;
    if (msg.has_readings()) {
        const auto& readings_msg = msg.readings();
        readings_len = static_cast<size_t>(readings_msg.data_size());
        readings_vec.reserve(readings_len);
        for (int i = 0; i < readings_msg.data_size(); ++i) {
            readings_vec.push_back(readings_msg.data(i));
        }
        if (!readings_vec.empty()) {
            readings_ptr = (const float *)readings_vec.data();
        }
    }
    process_data(error_code_ptr, readings_ptr, (int)readings_len, msg.label().c_str());
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
