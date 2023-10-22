#include <chrono>
#include <csignal>
#include <ctime>
#include <fcntl.h>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <sys/file.h>
#include <sys/stat.h>
#include <unistd.h>

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

    Matt_daemon() {
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
            throw std::runtime_error("Cannot obtain lock file");
        }

        instance = this;
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

    void listen() {
        while (running) {
            sleep(5);
        }
    }

    ~Matt_daemon() {
        close(fd);

        remove("/var/lock/matt_daemon.lock");
    }

  private:
    Tintin_reporter reporter;
    int fd;

    static void on_sigint(int signum) {
        instance->reporter.log("Received sigint signal", Tintin_reporter::Level::INFO);

        running = false;
    }
};

volatile bool Matt_daemon::running = true;
Matt_daemon *Matt_daemon::instance = nullptr;

int main() {
    try {
        Matt_daemon app;

        app.daemonize();
        app.listen();
    } catch (const std::runtime_error &e) {
        std::cerr << e.what() << std::endl;
    }

    return 0;
}
