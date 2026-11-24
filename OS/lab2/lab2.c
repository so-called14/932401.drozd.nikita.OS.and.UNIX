#define _GNU_SOURCE
#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

#define PORT 8080
#define BACKLOG 16
#define BUF_SIZE 4096

static volatile sig_atomic_t was_sighup = 0;
static void sighup_handler(int signal_number)
{
    (void)signal_number;
    was_sighup = 1;
}

int main(void)
{
    int exit_code = EXIT_FAILURE;

    int server_fd = -1;
    int client_fd = -1;
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("socket");
        return EXIT_FAILURE;
    }

    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        perror("setsockopt");
        goto remove;
    }

    struct sockaddr_in address;
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons(PORT);
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) == -1) {
        perror("bind");
        goto remove;
    }

    if (listen(server_fd, BACKLOG) == -1) {
        perror("listen");
        goto remove;
    }

    struct sigaction action;
    memset(&action, 0, sizeof(action));
    action.sa_handler = sighup_handler;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;
    if (sigaction(SIGHUP, &action, NULL) == -1) {
        perror("sigaction");
        goto remove;
    }

    sigset_t blocked_mask, original_mask, empty_mask;
    sigemptyset(&blocked_mask);
    sigaddset(&blocked_mask, SIGHUP);
    if (sigprocmask(SIG_BLOCK, &blocked_mask, &original_mask) == -1) {
        perror("sigprocmask");
        goto remove;
    }
    sigemptyset(&empty_mask);
    fprintf(stderr, "Сервер слушает порт %d\n", PORT);

    for (;;) {
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(server_fd, &read_fds);

        int max_fd = server_fd;
        if (client_fd != -1) {
            FD_SET(client_fd, &read_fds);
            if (client_fd > max_fd)
                max_fd = client_fd;
        }

        int ready = pselect(max_fd + 1, &read_fds, NULL, NULL, NULL, &empty_mask);

        if (ready == -1) {
            if (errno == EINTR) {
                if (was_sighup) {
                    was_sighup = 0;
                    fprintf(stderr, "Получен SIGHUP\n");
                }
                continue;
            }
            perror("pselect");
            goto remove;
        }

        if (was_sighup) {
            was_sighup = 0;
            fprintf(stderr, "Получен SIGHUP\n");
        }

        if (FD_ISSET(server_fd, &read_fds)) {
            int new_fd = accept(server_fd, NULL, NULL);
            if (new_fd == -1) {
                perror("accept");
                goto remove;
            }

            if (client_fd == -1) {
                client_fd = new_fd;
                fprintf(stderr, "Принято соединение (fd=%d), удерживаем\n", client_fd);
            } 
            else {
                fprintf(stderr, "Принято соединение (fd=%d), закрываем\n", new_fd);
                close(new_fd);
            }
        }

        if (client_fd != -1 && FD_ISSET(client_fd, &read_fds)) {
            char buffer[BUF_SIZE];
            ssize_t received = recv(client_fd, buffer, sizeof(buffer), 0);

            if (received > 0) {
                fprintf(stderr, "Получено %zd байт от клиента fd=%d\n", received, client_fd);
            } 
            else if (received == 0) {
                fprintf(stderr, "Клиент fd=%d закрыл соединение\n", client_fd);
                close(client_fd);
                client_fd = -1;
            } 
            else {
                perror("recv");
                close(client_fd);
                client_fd = -1;
            }
        }
    }

remove:
    if (client_fd != -1)
        close(client_fd);
    if (server_fd != -1)
        close(server_fd);
    sigprocmask(SIG_SETMASK, &original_mask, NULL);

    return exit_code;
}
