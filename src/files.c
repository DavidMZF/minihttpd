#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include "files.h"

/*
 * resolve_path: Construye la ruta absoluta de un archivo a partir de la URI
 * y verifica que este dentro de www_root.
 * Retorna 1 si la ruta es segura y el archivo existe.
 * Retorna 0 si el archivo no existe 
 * Retorna -1 si intenta salir de www_root.
 */
int resolve_path(const char *uri, char *real_path, const char *www_root) {
    char full_path[PATH_MAX];
    char real_root[PATH_MAX];

    snprintf(full_path, sizeof(full_path), "%s%s", www_root, uri);

    if (realpath(www_root, real_root) == NULL)
        return 0;

    if (realpath(full_path, real_path) == NULL) {
        char test[PATH_MAX];
        snprintf(test, sizeof(test), "%s", full_path);
        if (strstr(uri, "..") != NULL)
            return -1;
        return 0;
    }

    if (strncmp(real_path, real_root, strlen(real_root)) != 0)
        return -1;

    return 1;
}
