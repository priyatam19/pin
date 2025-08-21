# 0 "temp_no_pp.c"
# 0 "<built-in>"
# 0 "<command-line>"
# 1 "/usr/include/stdc-predef.h" 1 3 4
# 0 "<command-line>" 2
# 1 "temp_no_pp.c"



void checkNum(int N) {


    if (N == 0) {
        printf("Zeri\n");
    }

    else if (N < 0) {
        printf("Negative\n");
    }

    else {
        printf("Positive\n");
    }
}

int main() {
    int N = 10;
    checkNum(N);
    return 0;
}
