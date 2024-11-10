#define NOB_IMPLEMENTATION
#define NOB_STRIP_PREFIX
#include "src/nob.h"

int main(int argc, char **argv)
{
    NOB_GO_REBUILD_URSELF(argc, argv);
    Cmd cmd = {0};
    cmd_append(&cmd, "cc", "-Wall", "-Wextra", "-ggdb", "-o", "src/randomart", "src/randomart.c", "-lm");
    if (!cmd_run_sync_and_reset(&cmd)) return 1;

    cmd_append(&cmd, "rm", "-f", "nob.old");
    if (!cmd_run_sync_and_reset(&cmd)) return 1;

    cmd_append(&cmd, "./src/randomart");
    if (!cmd_run_sync_and_reset(&cmd)) return 1;
    return 0;
}
