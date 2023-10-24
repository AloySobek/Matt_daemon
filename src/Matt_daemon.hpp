#ifndef MATT_DAEMON
#define MATT_DAEMON

#include <arpa/inet.h>
#include <csignal>
#include <fcntl.h>
#include <sys/socket.h>
#include <unistd.h>

#include "Tintin_reporter.hpp"

#define MAX_CLIENTS 3
#define LISTEN_PORT 4242

class Matt_daemon {
  public:
    static volatile bool running;
    static Matt_daemon *instance;

    Tintin_reporter reporter;

    Matt_daemon();
    Matt_daemon(const Matt_daemon &other);
    Matt_daemon &operator=(const Matt_daemon &other);

    void daemonize();
    void loop();

    ~Matt_daemon();

  private:
    struct sockaddr_in addr;

    fd_set readfds;

    int clients_fds[MAX_CLIENTS];
    int server_fd;
    int fd;

    static void on_sigint(int signum);
};

#endif
