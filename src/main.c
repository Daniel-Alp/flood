#include <stdio.h>

int main() {
    FILE *fp = fopen("./test//scanning/numbers.txt", "r");
    int next;
    if (fp) {
        while((next = getc(fp)) != EOF) {
            printf("%c",next);
        }
    }
}