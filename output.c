#include <stdio.h>
#include <math.h>

int multiply(int a, int b) {
    return (a * b);
}

void main() {
    int x = 6;
    int y = 7;
    int result = multiply(x, y);
    printf("%d\n", result);
}

