#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sqlite3.h"

#define MAX_INPUT_SIZE (16 * 1024)

int main(int argc, char **argv) {
    static unsigned char buf[MAX_INPUT_SIZE + 1];
    size_t size;
    sqlite3 *db = NULL;
    char *errmsg = NULL;
    int rc;

#ifdef __AFL_HAVE_MANUAL_CONTROL
    __AFL_INIT();
#endif

    while (__AFL_LOOP(1000)) {
        /* Reset state here */

        size = fread(buf, 1, MAX_INPUT_SIZE, stdin);
        if (size == 0)
            continue;
        buf[size] = '\0';

        rc = sqlite3_open(":memory:", &db);
        if (rc != SQLITE_OK || !db)
            continue;

        rc = sqlite3_exec(db, (const char*)buf, NULL, NULL, &errmsg);

        if (errmsg)
            sqlite3_free(errmsg);
        if (db)
            sqlite3_close(db);
    }
    return 0;
}