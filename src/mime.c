#include <string.h>  /* strrchr(), strcmp() */
#include "mime.h"

/*
 * Tabla de extensiones y sus tipos MIME correspondientes.
 */
static const struct {
    const char *ext;
    const char *type;
} mime_table[] = {
    {".html", "text/html"},
    {".css",  "text/css"},
    {".js",   "application/javascript"},
    {".png",  "image/png"},
    {".jpg",  "image/jpeg"},
    {".jpeg", "image/jpeg"},
    {".txt",  "text/plain"},
    {NULL, NULL}
};

/*
 * Busca la extension del archivo en la tabla y retorna su tipo MIME.
 */
const char *get_mime_type(const char *path) {
    const char *ext = strrchr(path, '.');

    if (!ext)
        return NULL;

    for (int i = 0; mime_table[i].ext != NULL; i++) {
        if (strcmp(ext, mime_table[i].ext) == 0)
            return mime_table[i].type;
    }

    return NULL;
}
