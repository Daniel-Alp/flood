#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include "object.h"
#include "vm.h"
#include "api.h"
#include "debug.h"

int main(int argc, const char **argv) 
{
    if (argc != 2) {
        printf("Usage: ./flood [script]\n");
        return 1;
    }
    struct VM vm;
    init_vm(&vm, "/home/alp/Projects/flood/test/api/get_global/");
    do_file(&vm, argv[1]);
    i32 i = get_global(&vm, "./main.fl", "foo");
    struct FnObj *fn = AS_FN(vm.globals.values[i]);
    if (i != -1)
        run_vm(&vm, fn);
    release_vm(&vm);
}