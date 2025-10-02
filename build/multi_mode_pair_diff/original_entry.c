#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

extern int pin_wrapper_entry(const uint8_t *data, size_t len);

int main(int argc, char *argv[]) {
  if (argc != 2) {
    fprintf(stderr, "Usage: %s input.bin\n", argv[0]);
    return 1;
  }

  FILE *f = fopen(argv[1], "rb");
  if (!f) {
    perror("fopen");
    return 1;
  }
  if (fseek(f, 0, SEEK_END) != 0) {
    perror("fseek");
    fclose(f);
    return 1;
  }
  long len = ftell(f);
  if (len < 0) {
    perror("ftell");
    fclose(f);
    return 1;
  }
  rewind(f);

  uint8_t *buf = malloc((size_t)len);
  if (!buf) {
    perror("malloc");
    fclose(f);
    return 1;
  }
  size_t read = fread(buf, 1, (size_t)len, f);
  fclose(f);
  if (read != (size_t)len) {
    fprintf(stderr, "Short read (expected %ld, got %zu)\n", len, read);
    free(buf);
    return 1;
  }

  int rc = pin_wrapper_entry(buf, (size_t)len);
  free(buf);
  return rc;
}
