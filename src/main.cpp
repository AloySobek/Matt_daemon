#include <arpa/inet.h>
#include <chrono>
#include <csignal>
#include <ctime>
#include <errno.h>
#include <fcntl.h>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
#include <sys/errno.h>
#include <sys/file.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

#define MAX_CLIENTS 3
#define LISTEN_PORT 4242

// TODO: Ensure Coplien forms in each class

class Tintin_reporter {
  public:
    enum Level { DEBUG, INFO, WARN, ERROR, CRITICAL, MAX };

    Tintin_reporter() {
        if (mkdir("/var/log/matt_daemon", 0775) == -1) {
            if (errno != EEXIST) {
                throw std::runtime_error(
                    "/var/log/matt_daemon directory does not exists and cannot be created");
            }
        }

        file.open("/var/log/matt_daemon/matt_daemon.log", std::ofstream::app);

        if (!file.is_open()) {
            throw std::runtime_error("Log file cannot be created");
        }
    }

    void log(const char *message, Level level) {
        auto now = std::chrono::system_clock::now();
        std::time_t currentTime = std::chrono::system_clock::to_time_t(now);
        std::tm *localTime = std::localtime(&currentTime);

        file << "[" << std::setw(2) << std::setfill('0') << localTime->tm_mday << "/"
             << std::setw(2) << std::setfill('0') << (localTime->tm_mon + 1) << "/" << std::setw(4)
             << (localTime->tm_year + 1900) << "-" << std::setw(2) << std::setfill('0')
             << localTime->tm_hour << ":" << std::setw(2) << std::setfill('0') << localTime->tm_min
             << ":" << std::setw(2) << std::setfill('0') << localTime->tm_sec << "]"
             << " [ " << Tintin_reporter::level_to_string[level] << " ] - Matt_daemon: " << message
             << std::endl;
    }

    ~Tintin_reporter() {
        file.flush();
        file.close();
    }

  private:
    static constexpr char const *level_to_string[Level::MAX] = {"DEBUG", "INFO", "WARN", "ERROR",
                                                                "CRITICAL"};

    std::ofstream file;
};

class Matt_daemon {
  public:
    static volatile bool running;
    static Matt_daemon *instance;

    Tintin_reporter reporter;

    Matt_daemon() {
        instance = this;

        if (mkdir("/var/lock", 0775) == -1) {
            if (errno != EEXIST) {
                throw std::runtime_error(
                    "/var/lock directory does not exists and cannot be created");
            }
        }

        if ((fd = open("/var/lock/matt_daemon.lock", O_WRONLY | O_APPEND | O_CREAT)) == -1) {
            throw std::runtime_error("Lock file could not be opened");
        }

        if (flock(fd, LOCK_EX | LOCK_NB) == -1) {
            throw std::runtime_error("flock failed");
        }
    }

    void daemonize() {
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

    void loop() {
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
                        reporter.log(buffer, Tintin_reporter::Level::DEBUG);
                    }
                }
            }
        }
    }

    ~Matt_daemon() {
        for (int i = 0; i < MAX_CLIENTS; ++i) {
            close(clients_fds[i]);
        }

        flock(fd, LOCK_UN);

        close(fd);

        remove("/var/lock/matt_daemon.lock");
    }

  private:
    struct sockaddr_in addr;
    fd_set readfds;
    int fd;

    int clients_fds[MAX_CLIENTS];
    int server_fd;

    static void on_sigint(int signum) {
        instance->reporter.log("Received sigint signal", Tintin_reporter::Level::INFO);

        running = false;
    }
};

volatile bool Matt_daemon::running = true;
Matt_daemon *Matt_daemon::instance = nullptr;

int main() {
    Matt_daemon *app = new Matt_daemon();
    bool daemonized = false;

    try {
        app->daemonize();

        daemonized = true;

        app->loop();
    } catch (const std::runtime_error &e) {
        if (!daemonized) {
            std::cerr << e.what() << std::endl;
        } else {
            app->reporter.log(e.what(), Tintin_reporter::Level::CRITICAL);
        }
    }

    delete app;

    return 0;
}
