#ifndef HTTP_H
#define HTTP_H

/* Tamaños maximos para proteger contra buffer overflow */
#define MAX_METHOD_LEN   8
#define MAX_URI_LEN      2048
#define MAX_VERSION_LEN  16

/* Funcion principal que recibe el request y genera la respuesta */
void handle_request(int client_fd, char *buffer);

#endif
