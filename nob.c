#define NOB_IMPLEMENTATION
#include "nob.h"

#define BUILD_DIR "./build/"

int run(Nob_Cmd *cmd) {
    char *c;
    nob_cmd_append(cmd, BUILD_DIR"ced");

    if (!nob_cmd_run_sync_and_reset(cmd)) return 0;
    return 1;
}

int main(int argc, char **argv) {
    NOB_GO_REBUILD_URSELF(argc, argv);

    if (!nob_mkdir_if_not_exists(BUILD_DIR)) return 1;

    Nob_Cmd cmd = {0};
    nob_cmd_append(&cmd, "cc", "-Wall", "-Wextra", "-pedantic", "-std=c99");
    nob_cmd_append(&cmd, "main.c", "draw.c", "editor.c");
    nob_cmd_append(&cmd, "-o", BUILD_DIR"ced");
    nob_cmd_append(&cmd, "-lSDL2", "-lSDL2_ttf");

    if (!nob_cmd_run_sync_and_reset(&cmd)) return 1;

    char *name = nob_shift(argv, argc);

    if (argc > 0 && strcmp(nob_shift(argv, argc), "run") == 0) {
        if (!run(&cmd)) return 1;
    }

    return 0;
}
