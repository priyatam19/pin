#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <vector>

#include <google/protobuf/stubs/common.h>
#include "cpp_proto/input.pb.h"

extern "C" int mg_mqtt_next_sub(struct mg_mqtt_message * msg, struct mg_str * topic, uint8_t * qos, size_t pos);

int pin_reference_entry(const uint8_t *data, size_t len) {
    Input msg;
    if (!msg.ParseFromArray(data, static_cast<int>(len))) {
        return 1;
    }
    const auto& msg_msg = msg.msg();
    mg_mqtt_message msg_storage = {};
    struct mg_mqtt_message * msg_ptr = nullptr;
    if (msg_msg.has_value()) {
        msg_storage = msg_msg.value();
        msg_ptr = &msg_storage;
    }
    const auto& topic_msg = msg.topic();
    mg_str topic_storage = {};
    struct mg_str * topic_ptr = nullptr;
    if (topic_msg.has_value()) {
        topic_storage = topic_msg.value();
        topic_ptr = &topic_storage;
    }
    const auto& qos_msg = msg.qos();
    uint8_t qos_storage = 0;
    uint8_t * qos_ptr = nullptr;
    if (qos_msg.has_value()) {
        qos_storage = static_cast<uint8_t>(qos_msg.value());
        qos_ptr = &qos_storage;
    }
    mg_mqtt_next_sub(msg_ptr, topic_ptr, qos_ptr, msg.pos());
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
