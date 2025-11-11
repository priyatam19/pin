#include <stddef.h>

int sum_slice(const int *values, size_t count)
{
    if (!values || count == 0) {
        return 0;
    }

    int total = 0;
    for (size_t i = 0; i < count; ++i) {
        total += values[i];
    }
    return total;
}
