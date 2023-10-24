#include "Matt_daemon.hpp"

// Signal interrupts our code at any moment, these are only for signals
volatile bool Matt_daemon::running = true;
Matt_daemon *Matt_daemon::instance = nullptr;

Matt_daemon::Matt_daemon() {
    instance = this;

    if (mkdir("/var/lock", 0775) == -1) {
        if (errno != EEXIST) {
            throw std::runtime_error("/var/lock directory does not exists and cannot be created");
        }
    }

    if ((fd = open("/var/lock/matt_daemon.lock", O_WRONLY | O_APPEND | O_CREAT)) == -1) {
        throw std::runtime_error("Lock file could not be opened");
    }

    if (flock(fd, LOCK_EX | LOCK_NB) == -1) {
        throw std::runtime_error("flock failed");
    }
}

Matt_daemon::Matt_daemon(const Matt_daemon &other) {}

Matt_daemon &Matt_daemon::operator=(const Matt_daemon &other) { return *this; }

void Matt_daemon::daemonize() {
    pid_t pid = fork();

    if (pid < 0) {
        throw std::runtime_error("Can't daemonize, fork error");
    }

    if (pid > 0) {
        exit(0);
    }

    if (setsid() < 0) {
        throw std::runtime_error("setsid failed");
    }

    reporter.log("Setting up signal handlers...", Tintin_reporter::Level::INFO);

    std::signal(SIGINT, Matt_daemon::on_sigint);

    if (chdir("/") < 0) {
        throw std::runtime_error("chdir failed");
    }

    umask(0);

    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    reporter.log("Successfully daemonized.", Tintin_reporter::Level::INFO);
}

void Matt_daemon::loop() {
    memset(&addr, 0, sizeof(addr));
    memset(clients_fds, 0, sizeof(clients_fds));

    if ((server_fd = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
        throw std::runtime_error("socket failed");
    }

    addr.sin_len = sizeof(struct sockaddr_in);
    addr.sin_port = htons(LISTEN_PORT);
    addr.sin_family = AF_INET;
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        throw std::runtime_error(std::string("bind failed with the following error: ") +
                                 strerror(errno));
    }

    if (listen(server_fd, MAX_CLIENTS) < 0) {
        throw std::runtime_error("listen failed");
    }

    while (running) {
        int max_fd = server_fd;

        FD_ZERO(&readfds);

        FD_SET(server_fd, &readfds);
        for (int i = 0; i < MAX_CLIENTS; ++i) {
            if (clients_fds[i] > 0) {
                FD_SET(clients_fds[i], &readfds);

                max_fd = clients_fds[i] > max_fd ? clients_fds[i] : max_fd;
            }
        }

        if (select(max_fd + 1, &readfds, nullptr, nullptr, nullptr) < 0) {
            if (errno != EINTR) {
                reporter.log(
                    (std::string("select failed with the following error: ") + strerror(errno))
                        .c_str(),
                    Tintin_reporter::Level::WARN);
            }

            continue;
        }

        if (FD_ISSET(server_fd, &readfds)) {
            struct sockaddr client_addr;
            socklen_t client_addr_len = sizeof(client_addr);
            int client_fd;

            if ((client_fd = accept(server_fd, &client_addr, &client_addr_len)) < 0) {
                reporter.log(
                    (std::string("accept failed with the following error: ") + strerror(errno))
                        .c_str(),
                    Tintin_reporter::Level::WARN);

                continue;
            }

            for (int i = 0; i < MAX_CLIENTS; ++i) {
                if (clients_fds[i] == 0) {
                    clients_fds[i] = client_fd;

                    break;
                }
            }
        }

        for (int i = 0; i < MAX_CLIENTS; ++i) {
            if (FD_ISSET(clients_fds[i], &readfds)) {
                int len;
                char buffer[1024];

                len = read(clients_fds[i], buffer, 1023);

                if (len == 0) {
                    buffer[len] = '\0';

                    close(clients_fds[i]);

                    clients_fds[i] = 0;
                } else {
                    reporter.log((std::string("Received message: ") + buffer).c_str(),
                                 Tintin_reporter::Level::DEBUG);

                    if (!strcmp("quit", buffer)) {
                        reporter.log("Received quit command, exiting...",
                                     Tintin_reporter::Level::INFO);

                        running = false;

                        break;
                    }
                }
            }
        }
    }
}

Matt_daemon::~Matt_daemon() {
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        close(clients_fds[i]);
    }

    flock(fd, LOCK_UN);

    close(fd);

    remove("/var/lock/matt_daemon.lock");
}

void Matt_daemon::on_sigint(int signum) {
    instance->reporter.log("Received sigint signal", Tintin_reporter::Level::INFO);

    running = false;
}
