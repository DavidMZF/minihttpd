#ifndef SERVER_H
#define SERVER_H

#define PORT        8080
#define MAX_EVENTS  10
#define BUFFER_SIZE 8192
#define MAX_URI_LEN 2048

int  server_init(void);
void server_run(int server_fd);

#endif
