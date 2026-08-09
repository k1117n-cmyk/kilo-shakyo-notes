#include <stdio.h>

int main() {
    printf("char : %lu byte\n", sizeof(char));
    printf("int  : %lu bytes\n", sizeof(int));
    printf("int* : %lu bytes\n", sizeof(int*)); // ポインタのサイズ
    return 0;
}
