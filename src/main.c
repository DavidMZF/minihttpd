#include <stdio.h>
#include <stdlib.h>
#include "server.h"

int main(void) {
    int server_fd = server_init();
    if (server_fd == -1) {
        fprintf(stderr, "Error al iniciar el servidor\n");
        return EXIT_FAILURE;
    }

    server_run(server_fd);

    return EXIT_SUCCESS;
}
