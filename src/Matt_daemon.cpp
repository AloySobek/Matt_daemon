#include "Matt_daemon.hpp"

// Signal interrupts our code at any moment, these are only for signals
volatile bool Matt_daemon::running = true;
Matt_daemon *Matt_daemon::instance = nullptr;

Matt_daemon::Matt_daemon() : addr{0}, readfds{0}, clients_fds{0}, server_fd{0}, fd{0} {
    instance = this;

    init_logger();
    acquire_lock();
}

// This class contains data which is not shareable or it doesn't make sense to copy it
Matt_daemon::Matt_daemon(const Matt_daemon &other) {}

// This class contains data which is not shareable or it doesn't make sense to copy it
Matt_daemon &Matt_daemon::operator=(const Matt_daemon &other) {
    if (&other != this) {
    }

    return *this;
}

void Matt_daemon::daemonize() {
    pid_t pid = fork();

    if (pid < 0) {
        reporter->critical("fork failed");

        throw std::runtime_error("fork failed");
    }

    if (pid > 0) {
        exit(0);
    }

    if (setsid() < 0) {
        reporter->critical("setsid failed");

        throw std::runtime_error("setsid failed");
    }

    // SIGKILL cannot be handled, blocked or ignored
    // SIGSTOP cannot be handled, blocked or ignored
    //
    // SIGTRAP is used for debugging purposes, avoid catching so as not to interfere with debuggers

    // Termination signals, makes perfect sense to catch them
    std::signal(SIGHUP, Matt_daemon::signal_handler);
    std::signal(SIGINT, Matt_daemon::signal_handler);
    std::signal(SIGQUIT, Matt_daemon::signal_handler);
    std::signal(SIGTERM, Matt_daemon::signal_handler);

    // User defined signals, custom logic, catch and exit
    std::signal(SIGUSR1, Matt_daemon::signal_handler);
    std::signal(SIGUSR2, Matt_daemon::signal_handler);

    // Program error signals, makes perfect sense to catch them, log the error and exit, but it
    // could be tricky due to possible corrupted state of the program

    // Illegal CPU instruction due to corruption or something else
    std::signal(SIGILL, Matt_daemon::bad_signal_handler);
    // Indicates that the program is behaving abnormally, usually raised by the program itself
    std::signal(SIGABRT, Matt_daemon::bad_signal_handler);
    // Caused by bus error due to various memory access related issues
    std::signal(SIGBUS, Matt_daemon::bad_signal_handler);
    // Raised when the program tries to access restricted memory areas
    std::signal(SIGSEGV, Matt_daemon::bad_signal_handler);
    // Arithmetic exception due to various arithmetic issues such as division by zero, etc...
    std::signal(SIGFPE, Matt_daemon::bad_signal_handler);
    // Generated when the process tries to write to a pipe or a socket which doesn't have a reader
    std::signal(SIGPIPE, Matt_daemon::bad_signal_handler);

    // Received when child process terminates
    std::signal(SIGCHLD, Matt_daemon::signal_handler);
    // Simple timer
    std::signal(SIGALRM, Matt_daemon::signal_handler);
    // Obsolete signal which means stack fault
    // std::signal(SIGSTKFLT, Matt_daemon::signal_handler);
    // Continue the execution
    std::signal(SIGCONT, Matt_daemon::signal_handler);
    // Same as SIGSTOP but can be catched
    // std::signal(SIGSTP, Matt_daemon::signal_handler);
    // When background process tries to read from console(stdin) the OS sends this signal
    std::signal(SIGTTIN, Matt_daemon::signal_handler);
    // Same as SIGTIN but for stdout
    std::signal(SIGTTOU, Matt_daemon::signal_handler);
    // Generated when urgent, out of band data arrives to a socket
    std::signal(SIGURG, Matt_daemon::signal_handler);
    // Process has exceeded it's CPU budget
    std::signal(SIGXCPU, Matt_daemon::signal_handler);
    // Process has exceeded max file size limits
    std::signal(SIGXFSZ, Matt_daemon::signal_handler);
    // Same as SIGALRM but with CPU time, not real time
    std::signal(SIGVTALRM, Matt_daemon::signal_handler);
    // Used for profilers, useful to generate a log file when the profiler asks
    std::signal(SIGPROF, Matt_daemon::signal_handler);
    // Terminal size has changed
    std::signal(SIGWINCH, Matt_daemon::signal_handler);
    // Async io notification
    std::signal(SIGIO, Matt_daemon::signal_handler);
    // The system goes down, gives a chance to do something before the system shuts down
    // std::signal(SIGPWR, Matt_daemon::signal_handler);
    // Invalid system call
    std::signal(SIGSYS, Matt_daemon::signal_handler);

    if (chdir("/") < 0) {
        reporter->critical("chdir failed");

        throw std::runtime_error("chdir failed");
    }

    umask(0);

    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    reporter->info("Successfully daemonized");
}

void Matt_daemon::create_server() {
    reporter->debug("Creating server...");

    if ((server_fd = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
        reporter->critical("socket failed");

        throw std::runtime_error("socket failed");
    }

    int allow = 1;

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &allow, sizeof(allow)) < 0) {
        reporter->critical("setsockopt failed");

        throw std::runtime_error("setsockopt failed");
    }

    addr.sin_len = sizeof(struct sockaddr_in);
    addr.sin_port = htons(LISTEN_PORT);
    addr.sin_family = AF_INET;
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        reporter->critical(
            (std::string("bind failed with the following error: ") + strerror(errno)).c_str());

        throw std::runtime_error(std::string("bind failed with the following error: ") +
                                 strerror(errno));
    }

    if (listen(server_fd, MAX_CLIENTS) < 0) {
        reporter->critical("listen failed");

        throw std::runtime_error("listen failed");
    }

    reporter->debug("Server created");
}

void Matt_daemon::loop() {
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
                reporter->error(
                    (std::string("select failed with the following error: ") + strerror(errno))
                        .c_str());
            }

            continue;
        }

        if (FD_ISSET(server_fd, &readfds)) {
            struct sockaddr client_addr;
            socklen_t client_addr_len = sizeof(client_addr);
            int client_fd, i;

            for (i = 0; i < MAX_CLIENTS; ++i) {
                if (clients_fds[i] == 0) {
                    break;
                }
            }

            if ((client_fd = accept(server_fd, &client_addr, &client_addr_len)) < 0) {
                reporter->error(
                    (std::string("accept failed with the following error: ") + strerror(errno))
                        .c_str());
            } else {
                if (i < MAX_CLIENTS) {
                    reporter->info(("New client: " + std::to_string(client_fd)).c_str());

                    clients_fds[i] = client_fd;
                } else {
                    reporter->warn(("Client " + std::to_string(client_fd) +
                                    " has been rejected due to MAX_CLIENTS")
                                       .c_str());

                    close(client_fd);
                }
            }
        }

        for (int i = 0; i < MAX_CLIENTS; ++i) {
            if (clients_fds[i] > 0 && FD_ISSET(clients_fds[i], &readfds)) {
                int len;
                char buffer[1024];

                if ((len = read(clients_fds[i], buffer, 1023)) < 0) {
                    reporter->error(
                        (std::string("read failed with the following error: ") + strerror(errno))
                            .c_str());

                    continue;
                }

                buffer[len] = '\0';

                if (len == 0) {
                    reporter->info(
                        ("Client: " + std::to_string(clients_fds[i]) + " disconnected.").c_str());

                    close(clients_fds[i]);

                    clients_fds[i] = 0;
                } else {
                    reporter->info(
                        ("Received new command(" + std::to_string(len) + "): " + buffer).c_str());

                    if (!strcmp("quit", buffer)) {
                        reporter->info("Received quit command, exiting...");

                        running = false;

                        break;
                    }
                }
            }
        }
    }
}

void Matt_daemon::init_logger() {
    if (mkdir("/var/log/matt_daemon", 0775) == -1) {
        if (errno != EEXIST) {
            throw std::runtime_error(
                "/var/log/matt_daemon directory does not exists and cannot be created");
        }
    }

    auto file =
        std::make_shared<std::ofstream>("/var/log/matt_daemon/matt_daemon.log", std::ofstream::app);

    if (!file->is_open()) {
        throw std::runtime_error("Log file cannot be created/opened");
    }

    reporter.reset(new Tintin_reporter(file, "Matt_daemon"));

    reporter->set_level(Logger::Level::DEBUG);
}

void Matt_daemon::acquire_lock() {
    reporter->debug("Acquiring lock file...");

    if (mkdir("/var/lock", 0775) < 0) {
        if (errno != EEXIST) {
            reporter->critical("/var/lock directory does not exists and cannot be created");

            throw std::runtime_error("/var/lock directory does not exists and cannot be created");
        }
    }

    if ((fd = open("/var/lock/matt_daemon.lock", O_WRONLY | O_APPEND | O_CREAT)) < 0) {
        reporter->critical("Lock file could not be opened");

        throw std::runtime_error("Lock file could not be opened");
    }

    if (flock(fd, LOCK_EX | LOCK_NB) < 0) {
        reporter->critical("flock failed");

        throw std::runtime_error("flock failed");
    }

    reporter->debug("Lock file acquired");
}

void Matt_daemon::cleanup() {
    reporter->info("Gracefully shutting down...");

    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (clients_fds[i] > 0) {
            close(clients_fds[i]);
        }
    }

    close(server_fd);

    reporter->info("Server destroyed");

    flock(fd, LOCK_UN);

    close(fd);

    remove("/var/lock/matt_daemon.lock");

    reporter->info("Lock file released");

    reporter.reset(nullptr);
}

Matt_daemon::~Matt_daemon() { cleanup(); }

void Matt_daemon::signal_handler(int signum) {
    instance->reporter->warn(
        (std::string("Received '") + strsignal(signum) + "' signal, exiting...").c_str());

    running = false;
}

void Matt_daemon::bad_signal_handler(int signum) {
    instance->reporter->critical((std::string("Received critical '") + strsignal(signum) +
                                  "' signal, restoring default signal handler")
                                     .c_str());
    instance->cleanup();

    signal(signum, SIG_DFL);

    raise(signum);
}
