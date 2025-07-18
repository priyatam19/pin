#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sqlite3.h"

#define MAX_INPUT_SIZE 4096

int main(void) {
#ifdef __AFL_FUZZ_TESTCASE_LEN
  __AFL_INIT();
#endif

  unsigned char buf[MAX_INPUT_SIZE];
  size_t n;
  sqlite3 *db = NULL;
  char *errmsg = NULL;

#ifdef __AFL_LOOP
  while (__AFL_LOOP(10000)) {
#endif
    n = fread(buf, 1, sizeof(buf) - 1, stdin);
    if (n == 0)
      return 0;
    buf[n] = '\0';

    if (sqlite3_open(":memory:", &db) != SQLITE_OK)
      return 1;

    (void)sqlite3_exec(db, (const char *)buf, NULL, NULL, &errmsg);

    if (errmsg)
      sqlite3_free(errmsg);
    if (db)
      sqlite3_close(db);

    /* Reset state here */

#ifdef __AFL_LOOP
  }
#endif
  return 0;
}
/*
// === Normalized driver for CoverMe 2.0 ===
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>

double foo_r(const uint8_t *buf, uint64_t len) {
    // If no data, nothing to do
    if (len == 0) return 0.0;

    // Copy into a C‐string and null‐terminate
    char *sql = malloc(len + 1);
    if (!sql) return 0.0;
    memcpy(sql, buf, len);
    sql[len] = '\0';

    // Open in‐memory database
    sqlite3 *db = NULL;
    char *errmsg = NULL;
    if (sqlite3_open(":memory:", &db) != SQLITE_OK) {
        free(sql);
        return 0.0;
    }

    // Execute the SQL commands from the input buffer
    (void)sqlite3_exec(db, sql, NULL, NULL, &errmsg);

    // Cleanup
    if (errmsg)      sqlite3_free(errmsg);
    if (db)          sqlite3_close(db);
    free(sql);

    // Return a dummy score (coverage is tracked via instrumentation)
    return 0.0;
}
*/