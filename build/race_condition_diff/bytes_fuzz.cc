#include <cstdint>
#include <stddef.h> // for size_t in global namespace
extern "C" int pin_wrapper_entry(const uint8_t *data, size_t len);
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
  (void)pin_wrapper_entry(data, size);
  return 0;
}
