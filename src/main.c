#include <stdlib.h>
#include "parse.h"
#include "sema.h"
#include "compile.h"
#include "vm.h"

int main (int argc, char **argv) {
    if (argc != 2)
        exit(1);

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

    // for now just compile and execute the body of the first function
    // this is a hack but needed for testing
    struct Node *body = (struct Node*)((struct FnDeclNode*)node->stmts[0])->body;

    struct SemaState sema;
    init_sema_state(&sema);
    analyze(&sema, body);

    if (sema.errlist.count > 0) {
        print_errlist(&sema.errlist);
        goto err_release_sema_state;
    }

    struct Compiler compiler;
    init_compiler(&compiler);
    compile(&compiler, body, &sema.st);

    run(&compiler.chunk);

    release_compiler(&compiler);
err_release_sema_state:
    release_sema_state(&sema);
err_release_parser:
    release_parser(&parser);
    release_file_array(&file_arr);
}