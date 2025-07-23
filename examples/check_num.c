// C Program to check if a number is positive, negative,
// or zero using simple conditional checks
#include <stdio.h>

void checkNum(int N) {
  
    // Check if the number is zero
    if (N == 0) {
        printf("Zeri\n");
    }
    // Check if the number is less than zero
    else if (N < 0) {
        printf("Negative\n");
    }
    // If neither, the number is positive
    else {
        printf("Positive\n");
    }
}

int main() {
    int N = 10;
    checkNum(N);
    return 0;
}