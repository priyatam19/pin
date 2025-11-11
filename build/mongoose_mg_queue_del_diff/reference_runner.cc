#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <vector>

#include <google/protobuf/stubs/common.h>
#include "cpp_proto/input.pb.h"

extern "C" void mg_queue_del(struct mg_queue * q, size_t len);

int pin_reference_entry(const uint8_t *data, size_t len) {
    Input msg;
    if (!msg.ParseFromArray(data, static_cast<int>(len))) {
        return 1;
    }
    std::vector<mg_queue> q_vec;
    struct mg_queue * q_ptr = nullptr;
    size_t q_len = 0;
    if (msg.has_q()) {
        const auto& q_msg = msg.q();
        q_len = static_cast<size_t>(q_msg.data_size());
        q_vec.reserve(q_len);
        for (int i = 0; i < q_msg.data_size(); ++i) {
            q_vec.push_back(q_msg.data(i));
        }
        if (!q_vec.empty()) {
            q_ptr = (struct mg_queue *)q_vec.data();
        }
    }
    mg_queue_del(q_ptr, (size_t)q_len);
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
