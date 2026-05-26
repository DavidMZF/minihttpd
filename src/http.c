#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/sendfile.h> 
#include <limits.h>
#include "http.h"
#include "mime.h"
#include "files.h"

/* 
 * Carpeta con los recursos que el servidor puede entregar
 */
#define WWW_ROOT "./www"

/*
 * send_response: construye y envia una respuesta HTTP completa.
 */
static void send_response(int fd, int status, const char *status_text, const char *content_type, const char *body, int keep_alive) {
                           
    char headers[512];
    int body_len = body ? (int)strlen(body) : 0;

    snprintf(headers, sizeof(headers),
        "HTTP/1.1 %d %s\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %d\r\n"
        "Connection: %s\r\n"
        "\r\n",
        status, status_text,
        content_type,
        body_len,
        keep_alive ? "keep-alive" : "close"
    );

    write(fd, headers, strlen(headers));

    if (body && body_len > 0)
        write(fd, body, body_len);
}

/*
 * send_error: envia una respuesta de error con un mensaje HTML simple.
 */
static void send_error(int fd, int status, const char *status_text) {
    char body[128];

    snprintf(body, sizeof(body),
        "<html><body><h1>%d %s</h1></body></html>",
        status, status_text);

    send_response(fd, status, status_text, "text/html", body, 0);
}

/*
 * send_file: envia un archivo estatico al cliente. Usando sendfile(),
 * para transferir el archivo directamente sin copiarlo a memoria de usuario.
 */
static void send_file(int client_fd, const char *path, const char *mime_type, int keep_alive) {
    int file_fd = open(path, O_RDONLY);
    if (file_fd == -1) {
        send_error(client_fd, 404, "Not Found");
        return;
    }

    struct stat st;
    if (fstat(file_fd, &st) == -1) {
        send_error(client_fd, 500, "Internal Server Error");
        close(file_fd);
        return;
    }
    
    if (!S_ISREG(st.st_mode)) {
        send_error(client_fd, 403, "Forbidden");
        close(file_fd);
        return;
    }

    char headers[512];
    snprintf(headers, sizeof(headers),
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %ld\r\n"
        "Connection: %s\r\n"
        "\r\n",
        mime_type,
        st.st_size,
        keep_alive ? "keep-alive" : "close"
    );

    write(client_fd, headers, strlen(headers));

    sendfile(client_fd, file_fd, NULL, st.st_size);

    close(file_fd);
}

/*
 * parse_connection: lee el header "Connection:" del request.
 * Retorna 1 si el cliente quiere mantener la sesion, 0 si quiere cerrar.
 */
static int parse_connection(const char *buffer) {
    const char *conn = strstr(buffer, "Connection:");

    if (!conn)
        return 1;

    if (strncasecmp(conn, "Connection: close", 17) == 0)
        return 0;

    return 1;
}

/*
 * handle_request: Recibe el texto crudo del request, lo parsea y genera la respuesta.
 */
void handle_request(int client_fd, char *buffer) {

    char method[MAX_METHOD_LEN];
    char uri[MAX_URI_LEN];
    char version[MAX_VERSION_LEN];

    if (sscanf(buffer, "%7s %2047s %15s", method, uri, version) != 3) {
        send_error(client_fd, 400, "Bad Request");
        return;
    }

    if (strcmp(method, "GET") != 0) {
        send_error(client_fd, 405, "Method Not Allowed");
        return;
    }

    if (strcmp(version, "HTTP/1.1") != 0) {
        send_error(client_fd, 400, "Bad Request");
        return;
    }

    if (strcmp(uri, "/") == 0)
        strncpy(uri, "/index.html", sizeof(uri));

    char full_path[PATH_MAX];
    snprintf(full_path, sizeof(full_path), "%s%s", WWW_ROOT, uri);

    char real_path[PATH_MAX];
    if (!resolve_path(uri, real_path, WWW_ROOT)) {
        send_error(client_fd, 404, "Not Found");
        return;
}

    char real_root[PATH_MAX];
    if (realpath(WWW_ROOT, real_root) == NULL) {
        send_error(client_fd, 500, "Internal Server Error");
        return;
    }

    if (strncmp(real_path, real_root, strlen(real_root)) != 0) {
        send_error(client_fd, 403, "Forbidden");
        return;
    }

    const char *mime = get_mime_type(real_path);
    if (!mime) {
        send_error(client_fd, 415, "Unsupported Media Type");
        return;
    }
    
    int keep_alive = parse_connection(buffer);

    send_file(client_fd, real_path, mime, keep_alive);
}
