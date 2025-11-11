#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// Minimal replicas of the Mongoose string and MQTT message types to exercise
// the vulnerable parser logic without depending on the full library headers.
struct mg_str {
  const char *ptr;
  size_t len;
};

struct mg_mqtt_message {
  struct mg_str topic;
  struct mg_str data;
  struct mg_str dgram;
  uint16_t id;
  uint8_t cmd;
  uint8_t qos;
  uint8_t ack;
  size_t props_start;
  size_t props_size;
};

// Vulnerable implementation ported from mqtt-server example (Mongoose v7.10.3).
size_t mg_mqtt_next_topic(struct mg_mqtt_message *msg,
                          struct mg_str *topic,
                          uint8_t *qos,
                          size_t pos) {
  unsigned char *buf = (unsigned char *) msg->dgram.ptr + pos;
  size_t new_pos;
  if (pos >= msg->dgram.len) return 0;

  topic->len = (size_t) (((unsigned) buf[0]) << 8 | buf[1]);
  topic->ptr = (char *) buf + 2;
  new_pos = pos + 2 + topic->len + (qos == NULL ? 0 : 1);
  if ((size_t) new_pos > msg->dgram.len) return 0;
  if (qos != NULL) *qos = buf[2 + topic->len];
  return new_pos;
}
