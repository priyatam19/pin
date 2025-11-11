#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <vector>

#include <google/protobuf/stubs/common.h>
#include "cpp_proto/input.pb.h"

extern "C" int mg_mqtt_parse(const uint8_t * buf, size_t len, uint8_t version, struct mg_mqtt_message * m);

int pin_reference_entry(const uint8_t *data, size_t len) {
    Input msg;
    if (!msg.ParseFromArray(data, static_cast<int>(len))) {
        return 1;
    }
    std::vector<uint8_t> buf_vec;
    const uint8_t * buf_ptr = nullptr;
    size_t buf_len = 0;
    if (msg.has_buf()) {
        const auto& buf_msg = msg.buf();
        buf_len = static_cast<size_t>(buf_msg.data_size());
        buf_vec.reserve(buf_len);
        for (int i = 0; i < buf_msg.data_size(); ++i) {
            buf_vec.push_back(buf_msg.data(i));
        }
        if (!buf_vec.empty()) {
            buf_ptr = (const uint8_t *)buf_vec.data();
        }
    }
    const auto& m_msg = msg.m();
    mg_mqtt_message m_storage = {};
    struct mg_mqtt_message * m_ptr = nullptr;
    if (m_msg.has_value()) {
        m_storage = m_msg.value();
        m_ptr = &m_storage;
    }
    mg_mqtt_parse(buf_ptr, (size_t)buf_len, msg.version(), m_ptr);
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
