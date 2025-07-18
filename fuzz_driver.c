#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cJSON.h"

#ifndef MAX_INPUT_SIZE
#define MAX_INPUT_SIZE (1024 * 8)
#endif

int main(void) {
#ifdef __AFL_INIT
  __AFL_INIT();
#endif

  static unsigned char buf[MAX_INPUT_SIZE + 1];

#ifdef __AFL_LOOP
  while(__AFL_LOOP(1000)) {
#endif

    size_t bytes_read = fread(buf, 1, MAX_INPUT_SIZE, stdin);
    if(bytes_read == 0) {
#ifdef __AFL_LOOP
      continue;
#else
      return 0;
#endif
    }
    buf[bytes_read] = '\0';

    cJSON *json = cJSON_Parse((const char *)buf);
    if(json == NULL) {
#ifdef __AFL_LOOP
      continue;
#else
      return 0;
#endif
    }

    cJSON *replacement = cJSON_CreateNumber(123);

    if(json->type == 16 && json->valuestring != NULL) {
      cJSON_ReplaceItemInObject(json, "dummy", replacement);
    } else {
      if(json->type == 2 && cJSON_GetArraySize(json) > 0) {
        cJSON *first = json->child;
        if(first && first->string) {
          cJSON_ReplaceItemInObject(json, first->string, replacement);
        }
      }
    }

    cJSON_Delete(json);

    if (replacement->prev == replacement && replacement->next == replacement && !replacement->string) {
      cJSON_Delete(replacement);
    }

#ifdef __AFL_LOOP
  }
#endif

  return 0;
}