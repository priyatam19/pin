#include <stddef.h>

int scale_optional(int *value, int scale)
{
    if (!value) {
        return 0;
    }
    *value *= scale;
    return *value;
}
