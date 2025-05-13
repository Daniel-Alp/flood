#include <stdio.h>
#include <stdlib.h>
#include "debug.h"
#include "parse.h"
#include "sema.h"
#include "compile.h"
#include "vm.h"

int main (int argc, char **argv) {
    if (argc != 2)
        exit(1);

    printf("%s\n", argv[1]);

    struct Parser parser;
    init_parser(&parser);
    struct FileArr file_arr;
    init_file_array(&file_arr);
    
    parse(&parser, &file_arr, argv[1]);

    if (parser.errlist.count > 0) {
        print_errlist(&parser.errlist);
        goto err_release_parser;
    }

    struct FileNode *node = file_arr.files[0];

    struct SemaState sema;
    init_sema_state(&sema);
    analyze(&sema, node);

    if (sema.errlist.count > 0) {
        print_errlist(&sema.errlist);
        goto err_release_sema_state;
    }

err_release_sema_state:
    release_sema_state(&sema);
err_release_parser:
    release_parser(&parser);
    release_file_array(&file_arr);
}