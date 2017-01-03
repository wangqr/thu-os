#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define N 1000000

uint32_t data[N];

int uint_comp(const void *a, const void *b) {
    const uint32_t *pa = (const uint32_t *) a, *pb = (const uint32_t *) b;
    return (*pa < *pb) ? -1 : (*pa > *pb);
}

int main(void) {
    FILE* fp = fopen("unsorted", "r");
    if (N != fread(data, 4, N, fp)){
        puts("Input file load failed.");
        exit(EXIT_FAILURE);
    }
    fclose(fp);

    qsort(data, 1000000, 4, uint_comp);

    fp = fopen("sorted_std", "w");
    fwrite(data, 4, N, fp);
    fclose(fp);
    return EXIT_SUCCESS;
}