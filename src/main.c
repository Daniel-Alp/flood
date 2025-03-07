#include <stdio.h>
#include "memory.h"
#include "parser.h"

int main() {    
    struct Chunk chunk;
    init_chunk(&chunk);
    for (int i = 0; i < 100; i++) {
        write_chunk(&chunk, i);
    }
    for (int i = 0; i < 100; i++) {
        printf("%d\n", chunk.code[i]);
    }
    free_chunk(&chunk);
}