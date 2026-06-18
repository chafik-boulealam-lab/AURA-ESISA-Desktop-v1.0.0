#include <stdio.h>

int main() {
    FILE *f = fopen("test_output.txt", "w");
    fprintf(f, "Hello from test program\n");
    fclose(f);
    return 0;
}
