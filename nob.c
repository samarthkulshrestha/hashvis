#define NOB_IMPLEMENTATION
#define NOB_STRIP_PREFIX
#include "src/nob.h"

int main(int argc, char **argv)
{
    NOB_GO_REBUILD_URSELF(argc, argv);
    Cmd cmd = {0};
    cmd_append(&cmd, "cc", "-Wall", "-Wextra", "-ggdb", "-o", "src/randomart", "src/randomart.c");
    if (!cmd_run_sync(cmd)) return 1;
    return 0;
}
