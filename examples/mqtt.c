// Complete MQTT server demonstrating basic mqtt functionality
// Based on the mongoose MQTT server example with heap allocation

#include "mongoose.h"
#include <signal.h>
#include <stdlib.h>
#include <string.h>

static const char *s_listen_on = "mqtt://0.0.0.0:1883";

// A list of subscriptions, held in memory
struct sub {
  struct sub *next;
  struct mg_connection *c;
  struct mg_str topic;
  uint8_t qos;
};
static struct sub *s_subs = NULL;

// Handle interrupts, like Ctrl-C
static int s_signo;
static void signal_handler(int signo) {
  s_signo = signo;
}

// Helper functions for MQTT topic parsing
static size_t mg_mqtt_next_topic(struct mg_mqtt_message *msg,
                                 struct mg_str *topic, uint8_t *qos,
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

size_t mg_mqtt_next_sub(struct mg_mqtt_message *msg, struct mg_str *topic,
                        uint8_t *qos, size_t pos) {
  uint8_t tmp;
  return mg_mqtt_next_topic(msg, topic, qos == NULL ? &tmp : qos, pos);
}

size_t mg_mqtt_next_unsub(struct mg_mqtt_message *msg, struct mg_str *topic,
                          size_t pos) {
  return mg_mqtt_next_topic(msg, topic, NULL, pos);
}

// Process parsed MQTT message
static void process_mqtt_message(struct mg_connection *c, struct mg_mqtt_message *mm) {
  switch (mm->cmd) {
    case MQTT_CMD_CONNECT: {
      // Client connects
      MG_INFO(("CONNECT from %p", c->fd));
      if (mm->dgram.len < 9) {
        mg_error(c, "Malformed MQTT frame");
      } else {
        // Extract protocol version from the CONNECT packet
        uint8_t version = mm->dgram.ptr[8];
        MG_INFO(("MQTT version %d", version));
        
        // Accept connection
        uint8_t response[] = {0, 0};  // Connection accepted
        mg_mqtt_send_header(c, MQTT_CMD_CONNACK, 0, sizeof(response));
        mg_send(c, response, sizeof(response));
        
        // Store the version for this connection (in real code, you'd store this properly)
        c->data[0] = version;  // Hack: store version in data
      }
      break;
    }
    
    case MQTT_CMD_SUBSCRIBE: {
      // Client subscribes
      MG_INFO(("SUBSCRIBE from %p", c->fd));
      size_t pos = 4;  // Initial topic offset, where ID ends
      uint8_t qos, resp[256];
      struct mg_str topic;
      int num_topics = 0;
      
      while ((pos = mg_mqtt_next_sub(mm, &topic, &qos, pos)) > 0) {
        struct sub *sub = calloc(1, sizeof(*sub));
        sub->c = c;
        sub->topic = mg_strdup(topic);
        sub->qos = qos;
        LIST_ADD_HEAD(struct sub, &s_subs, sub);
        MG_INFO(("SUB %p [%.*s] QoS %d", c->fd, (int) sub->topic.len, sub->topic.ptr, qos));
        
        // Change '+' to '*' for topic matching using mg_match
        for (size_t i = 0; i < sub->topic.len; i++) {
          if (sub->topic.ptr[i] == '+') ((char *) sub->topic.ptr)[i] = '*';
        }
        resp[num_topics++] = qos;
      }
      
      // Send SUBACK
      mg_mqtt_send_header(c, MQTT_CMD_SUBACK, 0, num_topics + 2);
      uint16_t id = mg_htons(mm->id);
      mg_send(c, &id, 2);
      mg_send(c, resp, num_topics);
      break;
    }
    
    case MQTT_CMD_PUBLISH: {
      // Client published message
      MG_INFO(("PUBLISH from %p [%.*s] -> [%.*s]", c->fd, 
               (int) mm->data.len, mm->data.ptr,
               (int) mm->topic.len, mm->topic.ptr));
      
      // Send PUBACK for QoS 1 BEFORE forwarding
      if (mm->qos == 1) {
        uint16_t id = mg_htons(mm->id);
        mg_mqtt_send_header(c, MQTT_CMD_PUBACK, 0, 2);
        mg_send(c, &id, 2);
        MG_INFO(("Sent PUBACK for packet ID %d", mm->id));
      }
      
      // Forward to all matching subscribers
      for (struct sub *sub = s_subs; sub != NULL; sub = sub->next) {
        if (mg_match(mm->topic, sub->topic, NULL)) {
          MG_INFO(("Forwarding to %p", sub->c->fd));
          struct mg_mqtt_opts pub_opts;
          memset(&pub_opts, 0, sizeof(pub_opts));
          pub_opts.topic = mm->topic;
          pub_opts.message = mm->data;
          pub_opts.qos = sub->qos;
          pub_opts.retain = false;
          mg_mqtt_pub(sub->c, &pub_opts);
        }
      }
      break;
    }
    
    case MQTT_CMD_PINGREQ: {
      MG_INFO(("PINGREQ from %p", c->fd));
      mg_mqtt_send_header(c, MQTT_CMD_PINGRESP, 0, 0);
      MG_INFO(("Sent PINGRESP to %p", c->fd));
      break;
    }
    
    case MQTT_CMD_DISCONNECT: {
      MG_INFO(("DISCONNECT from %p", c->fd));
      c->is_closing = 1;
      break;
    }
    
    default:
      MG_INFO(("Unknown MQTT command %d from %p", mm->cmd, c->fd));
      break;
  }
}

// Event handler function
static void fn(struct mg_connection *c, int ev, void *ev_data, void *fn_data) {
  if (ev == MG_EV_READ) {
    // Process raw data instead of relying on MG_EV_MQTT_CMD
    size_t len = c->recv.len;
    
    if (len > 0) {
      MG_INFO(("Received %d bytes from %p", (int)len, c->fd));
      
      // Allocate on heap
      uint8_t *heap_buf = (uint8_t *)malloc(len);
      if (!heap_buf) {
        MG_ERROR(("Failed to allocate memory"));
        return;
      }
      
      // Copy data to heap
      memcpy(heap_buf, c->recv.buf, len);
      
      // Debug: hex dump first few bytes
      if (len <= 20) {
        printf("Data: ");
        for (size_t i = 0; i < len; i++) {
          printf("%02x ", heap_buf[i]);
        }
        printf("\n");
      }
      
      // Parse MQTT message from heap buffer
      struct mg_mqtt_message mm;
      
      // Determine MQTT version - default to 4, but use 5 if necessary
      uint8_t version = 4;
      if (c->data[0] == 5) {
        version = 5;  // Use stored version
      } else if (len == 8 && heap_buf[0] == 0x30 && heap_buf[1] == 0x06) {
        version = 5;
      }
      
      MG_INFO(("Parsing with MQTT version %d", version));
      int result = mg_mqtt_parse(heap_buf, len, version, &mm);
      
      if (result == MQTT_OK) {
        MG_INFO(("Successfully parsed MQTT message"));
        process_mqtt_message(c, &mm);
        mg_iobuf_del(&c->recv, 0, mm.dgram.len);
      } else if (result == MQTT_INCOMPLETE) {
        MG_INFO(("Incomplete MQTT message, waiting for more data"));
      } else {
        MG_INFO(("MQTT parse error %d", result));
        // For demo purposes, still try to continue
        mg_iobuf_del(&c->recv, 0, len);
      }
      
      // Free heap buffer
      free(heap_buf);
    }
  } else if (ev == MG_EV_ACCEPT) {
    MG_INFO(("New connection accepted from %p", c->fd));
    // Initialize connection - default to MQTT v4
    c->data[0] = 4;
  } else if (ev == MG_EV_CLOSE) {
    MG_INFO(("Connection %p closed", c->fd));
    // Remove subscriptions for this connection
    for (struct sub *next, *sub = s_subs; sub != NULL; sub = next) {
      next = sub->next;
      if (c != sub->c) continue;
      MG_INFO(("Removing subscription [%.*s] for %p", 
               (int) sub->topic.len, sub->topic.ptr, c->fd));
      LIST_DELETE(struct sub, &s_subs, sub);
      free((void *)sub->topic.ptr);
      free(sub);
    }
  }
  (void) ev_data;
  (void) fn_data;
}

int main(void) {
  struct mg_mgr mgr;
  signal(SIGINT, signal_handler);
  signal(SIGTERM, signal_handler);
  mg_mgr_init(&mgr);
  
  printf("================================================\n");
  printf("---------------- MQTT Server --------------------\n");
  printf("================================================\n");
  printf("This is a  MQTT server that demonstrates\n");
  printf("basic mqtt functionality with mongoose v7.10.3\n");
  printf("\n");
  printf("Features:\n");
  printf("- CONNECT/CONNACK handling\n");
  printf("- SUBSCRIBE/SUBACK with topic management\n");
  printf("- PUBLISH message forwarding\n");
  printf("- PINGREQ/PINGRESP keep-alive\n");
  printf("\n");
  printf("Listening on %s\n", s_listen_on);
  printf("================================================\n\n");
  
  mg_mqtt_listen(&mgr, s_listen_on, fn, NULL);
  
  while (s_signo == 0) mg_mgr_poll(&mgr, 1000);
  
  // Cleanup
  for (struct sub *next, *sub = s_subs; sub != NULL; sub = next) {
    next = sub->next;
    free((void *)sub->topic.ptr);
    free(sub);
  }
  
  mg_mgr_free(&mgr);
  return 0;
}