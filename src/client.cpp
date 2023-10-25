#include <arpa/inet.h>
#include <iostream>
#include <stdexcept>
#include <string>
#include <sys/socket.h>
#include <unistd.h>

#include "Tintin_reporter.hpp"

#define CONNECT_PORT 4242

class Ben_AFK {
  public:
    Ben_AFK(Tintin_reporter &reporter) : reporter{reporter} {
        if ((client_fd = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
            throw std::runtime_error("socket failed");
        }
    }
    Ben_AFK(const Ben_AFK &other) {}
    Ben_AFK &operator=(const Ben_AFK &other) { return *this; }

    void conn() {
        struct sockaddr_in addr;

        memset(&addr, 0, sizeof(addr));

        addr.sin_len = sizeof(addr);
        addr.sin_port = htons(CONNECT_PORT);
        addr.sin_family = AF_INET;
        inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

        if (connect(client_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
            throw std::runtime_error("connect failed");
        }
    }

    void loop() {
        while (true) {
            std::cout << ">";

            std::string line;

            std::getline(std::cin, line);

            std::cout << "Your command is: " << line << std::endl;

            if (send(client_fd, line.c_str(), line.length(), 0) < 0) {
                std::cout << "send failed" << std::endl;

                continue;
            }
        }

        close(client_fd);
    }

    ~Ben_AFK() {}

  private:
    Tintin_reporter reporter;

    int client_fd;
};

int main() {
    Tintin_reporter reporter(true);

    try {
        Ben_AFK client(reporter);

        client.conn();
        client.loop();
    } catch (const std::runtime_error &e) {
        reporter.log_stdout(e.what(), Tintin_reporter::Level::ERROR);
    }

    return 0;
}
