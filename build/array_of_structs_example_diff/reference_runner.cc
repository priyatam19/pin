#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <vector>

#include <google/protobuf/stubs/common.h>
#include "cpp_proto/input.pb.h"

extern "C" float average_samples(const struct Sample * samples, int count);

int pin_reference_entry(const uint8_t *data, size_t len) {
    Input msg;
    if (!msg.ParseFromArray(data, static_cast<int>(len))) {
        return 1;
    }
    std::vector<Sample> samples_vec;
    const struct Sample * samples_ptr = nullptr;
    size_t samples_len = 0;
    if (msg.has_samples()) {
        const auto& samples_msg = msg.samples();
        samples_len = static_cast<size_t>(samples_msg.data_size());
        samples_vec.reserve(samples_len);
        for (int i = 0; i < samples_msg.data_size(); ++i) {
            samples_vec.push_back(samples_msg.data(i));
        }
        if (!samples_vec.empty()) {
            samples_ptr = (const struct Sample *)samples_vec.data();
        }
    }
    average_samples(samples_ptr, (int)samples_len);
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
