#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
    printf("Simple CLI program called with:\n");
    printf("argc = %d\n", argc);
    
    for (int i = 0; i < argc; i++) {
        printf("argv[%d] = \"%s\"\n", i, argv[i]);
    }
    
    if (argc > 1) {
        printf("First argument: %s\n", argv[1]);
    }
    
    return argc; // Return argc as simple test
}