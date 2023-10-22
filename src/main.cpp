#include <fcntl.h>
#include <iostream>
#include <stdexcept>
#include <sys/file.h>
#include <sys/stat.h>
#include <unistd.h>

class Tintin_reporter {};

class Matt_daemon {
  public:
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

        std::cout << "Successfully obtained lock file, daemonazing..." << std::endl;
    }

    void daemonize() {}

    ~Matt_daemon() { close(fd); }

  private:
    int fd;
};

int main() {
    try {
        Matt_daemon app;

        app.daemonize();
    } catch (const std::runtime_error &e) {
        std::cout << e.what() << std::endl;
    }

    return 0;
}
