#include <cstdint>
#include <cstddef>

extern "C" int pin_wrapper_entry(const uint8_t *data, size_t size);

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
  pin_wrapper_entry(data, size);
  return 0;
}
