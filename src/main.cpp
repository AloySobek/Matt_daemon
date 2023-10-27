#include <iostream>
#include <memory>
#include <stdexcept>

#include "Matt_daemon.hpp"
#include "Tintin_reporter.hpp"

int main() {
    bool daemonized = false;

    try {
        Matt_daemon app;

        app.daemonize(), daemonized = true;

        app.create_server();

        app.loop();
    } catch (const std::runtime_error &e) {
        if (!daemonized) {
            std::cerr << e.what() << std::endl;
        }
    }

    return 0;
}
