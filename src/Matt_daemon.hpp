#ifndef MATT_DAEMON
#define MATT_DAEMON

#include <arpa/inet.h>
#include <csignal>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <memory>
#include <sys/file.h>
#include <sys/socket.h>
#include <unistd.h>

#include "Tintin_reporter.hpp"

#define MAX_CLIENTS 3
#define LISTEN_PORT 4242

// Matt_daemon class contains static variables which means you can't copy it, copy constructor and
// operator= are present only because of Coplien forms
class Matt_daemon {
  public:
    static volatile bool running;
    static Matt_daemon *instance;

    std::unique_ptr<Tintin_reporter> reporter;

    Matt_daemon();

    void daemonize();
    void create_server();
    void loop();

    ~Matt_daemon();

  private:
    struct sockaddr_in addr;

    fd_set readfds;

    int clients_fds[MAX_CLIENTS];
    int server_fd;
    int fd;

    Matt_daemon(const Matt_daemon &other);
    Matt_daemon &operator=(const Matt_daemon &other);

    void init_logger();
    void acquire_lock();
    void cleanup();

    // Default behavior, receive and gracefully shutdown
    static void signal_handler(int signum);
    // When the program receives potentially corrupting signals, bus error, seg. fault, etc...
    static void bad_signal_handler(int signum);
};

#endif
