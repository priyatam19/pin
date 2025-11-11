#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <vector>

#include <google/protobuf/stubs/common.h>
#include "cpp_proto/input.pb.h"

extern "C" int process_file(int * fts, int * ent);

int pin_reference_entry(const uint8_t *data, size_t len) {
    Input msg;
    if (!msg.ParseFromArray(data, static_cast<int>(len))) {
        return 1;
    }
    const auto& fts_msg = msg.fts();
    int fts_storage = 0;
    int * fts_ptr = nullptr;
    if (fts_msg.has_value()) {
        fts_storage = static_cast<int>(fts_msg.value());
        fts_ptr = &fts_storage;
    }
    const auto& ent_msg = msg.ent();
    int ent_storage = 0;
    int * ent_ptr = nullptr;
    if (ent_msg.has_value()) {
        ent_storage = static_cast<int>(ent_msg.value());
        ent_ptr = &ent_storage;
    }
    process_file(fts_ptr, ent_ptr);
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
