#include <arpa/inet.h>
#include <iostream>
#include <string>
#include <sys/socket.h>

#define CONNECT_PORT 4242

int main() {
    int client_fd = 0;

    if ((client_fd = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
        std::cout << "socket failed" << std::endl;

        return -1;
    }

    struct sockaddr_in addr;

    memset(&addr, 0, sizeof(addr));

    addr.sin_len = sizeof(addr);
    addr.sin_port = htons(CONNECT_PORT);
    addr.sin_family = AF_INET;
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

    if (connect(client_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        std::cout << "connect failed" << std::endl;

        return -1;
    }

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

    return 0;
}
