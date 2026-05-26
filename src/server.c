#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "server.h"
#include "http.h"

/*
 * set_nonblocking: Pone un file descriptor en modo no bloqueante.
 * Necesario para implementar epoll.
 */
static int set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
        perror("fcntl F_GETFL");
        return -1;
    }

    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        perror("fcntl F_SETFL");
        return -1;
    }

    return 0;
}

/*
 * server_init: Crear y configurar el socket del servidor.
 * Retorna el file descriptor del socket, o -1 en caso de error.
 */
int server_init(void) {
    int server_fd;
    struct sockaddr_in addr;
    int opt = 1;

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("socket");
        return -1;
    }

    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    memset(&addr, 0, sizeof(addr));
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port        = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        perror("bind");
        close(server_fd);
        return -1;
    }

    if (listen(server_fd, SOMAXCONN) == -1) {
        perror("listen");
        close(server_fd);
        return -1;
    }

    set_nonblocking(server_fd);
    printf("Servidor escuchando en http://localhost:%d\n", PORT);
    return server_fd;
}

/*
 * server_run: Bucle principal del servidor usando epoll.
 * Atiende nuevas conexiones y requests de clientes de forma concurrente.
 */
void server_run(int server_fd) {
    int epoll_fd;
    struct epoll_event ev;
    struct epoll_event events[MAX_EVENTS];

    epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        perror("epoll_create1");
        return;
    }

    ev.events  = EPOLLIN;
    ev.data.fd = server_fd;

    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &ev) == -1) {
        perror("epoll_ctl");
        close(epoll_fd);
        return;
    }
    
    printf("Esperando conexiones...\n");

    while (1) {
        int n = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        if (n == -1) {
            perror("epoll_wait");
            break;
        }

        for (int i = 0; i < n; i++) {
            int fd = events[i].data.fd;

            if (fd == server_fd) {
                struct sockaddr_in client_addr;
                socklen_t client_len = sizeof(client_addr);

                int client_fd = accept(server_fd,
                                       (struct sockaddr *)&client_addr,
                                       &client_len);
                if (client_fd == -1) {
                    perror("accept");
                    continue;
                }

                set_nonblocking(client_fd);

                ev.events  = EPOLLIN | EPOLLET;
                ev.data.fd = client_fd;

                if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &ev) == -1) {
                    perror("epoll_ctl: client_fd");
                    close(client_fd);
                    continue;
                }

                printf("Cliente conectado: %s (fd=%d)\n",
                       inet_ntoa(client_addr.sin_addr), client_fd);

            } else {
                char buffer[BUFFER_SIZE];
                memset(buffer, 0, sizeof(buffer));

                ssize_t bytes = read(fd, buffer, sizeof(buffer) - 1);

                if (bytes <= 0) {
                    if (bytes == 0)
                        printf("Cliente desconectado (fd=%d)\n", fd);
                    else if (errno != EAGAIN)
                        perror("read");

                    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL);
                    close(fd);
                    continue;
                }

                if (bytes >= MAX_URI_LEN) {
                    const char *err =
                        "HTTP/1.1 400 Bad Request\r\n"
                        "Content-Type: text/html\r\n"
                        "Content-Length: 15\r\n"
                        "Connection: close\r\n"
                        "\r\n"
                        "400 Bad Request";
                    write(fd, err, strlen(err));
                    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL);
                    close(fd);
                    continue;
                }

                handle_request(fd, buffer);
            }
        }
    }

    close(epoll_fd);
}
