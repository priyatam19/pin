typedef struct {
    char s[4096];
} Input;

#include <stdio.h>
#include "cJSON.h"

int P(const char* s) {
    cJSON *json = cJSON_Parse(s);
    if (!json) {
        printf("Parse error!\n");
        return 1;
    }
    printf("Valid JSON!\n");
    cJSON_Delete(json);
    return 0;
}