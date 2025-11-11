#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

struct mg_mgr;
struct mg_connection;
struct mg_mqtt_prop;
struct mg_mqtt_message;
struct mg_iobuf;
struct mg_str {
  const char *ptr;
  size_t len;
};
typedef void (*mg_event_handler_t)(struct mg_connection *, int, void *, void *);

size_t mg_send(struct mg_connection *c, const void *buf, size_t len) {
  (void)c;
  (void)buf;
  return len;
}

uint32_t mg_random(void) { return 0; }

void mg_hex(const void *buf, size_t len, char *to) {
  (void)buf;
  (void)len;
  if (to) *to = '\0';
}

struct mg_str mg_str_s(const char *s) {
  struct mg_str str;
  str.ptr = s;
  str.len = s ? (size_t)0 : 0;
  return str;
}

uint16_t mg_ntohs(uint16_t x) { return x; }
uint32_t mg_ntohl(uint32_t x) { return x; }

struct mg_connection *mg_connect(struct mg_mgr *mgr, const char *url,
                                 mg_event_handler_t fn, void *fn_data) {
  (void)mgr;
  (void)url;
  (void)fn;
  (void)fn_data;
  return (struct mg_connection *)0;
}

void mg_call(struct mg_connection *c, int ev, void *evd, void *fnd) {
  (void)c;
  (void)ev;
  (void)evd;
  (void)fnd;
}

void mg_iobuf_del(struct mg_iobuf *io, size_t ofs, size_t len) {
  (void)io;
  (void)ofs;
  (void)len;
}

struct mg_connection *mg_listen(struct mg_mgr *mgr, const char *url,
                                mg_event_handler_t fn, void *fn_data) {
  (void)mgr;
  (void)url;
  (void)fn;
  (void)fn_data;
  return (struct mg_connection *)0;
}

int mg_snprintf(char *buf, size_t len, const char *fmt, ...) {
  (void)buf;
  (void)len;
  (void)fmt;
  return 0;
}

int mg_vmprintf(char **buf, size_t len, const char *fmt, va_list ap) {
  (void)buf;
  (void)len;
  (void)fmt;
  (void)ap;
  return 0;
}
