#include "compile.h"
#include "debug.h"
#include "parse.h"
#include "sema.h"
#include <unistd.h> // for isatty()

int main(int argc, const char **argv)
{
    if (argc != 2) {
        printf("usage: flood script.fl\n");
        return 0;
    }

    FILE *fp = fopen(argv[1], "rb");
    fseek(fp, 0, SEEK_END);
    const u64 length = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    char *buf = new char[length + 2];
    buf[0] = '\0';
    buf[length + 1] = '\0';
    char *source = buf + 1;
    // TODO check for null bytes
    fread(source, 1, length, fp);
    const bool flag_color = isatty(1);

    Dynarr<ErrMsg> errarr;
    Arena arena;
    ModuleNode &node = Parser::parse(source, arena, errarr);
    if (errarr.len() > 0) {
        print_errarr(errarr, flag_color);
        delete[] buf;
        fclose(fp);
        return 1;
    }

    analyze(node, errarr, arena);

    if (errarr.len() > 0) {
        print_errarr(errarr, flag_color);
        delete[] buf;
        fclose(fp);
        return 1;
    }

    // print_module(node, true);

    VM vm;
    ClosureObj *script = compile(vm, node, errarr);
    if (errarr.len() > 0) {
        print_errarr(errarr, flag_color);
        delete[] buf;
        fclose(fp);
        return 1;
    }
    // FIXME closures at runtime
    // FIXME fnbody blocks popping `self` before it can be grabbed

    if (script)
        run_vm(vm, *script);

    delete[] buf;
    fclose(fp);
}
