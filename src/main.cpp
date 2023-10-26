#include <iostream>
#include <memory>
#include <stdexcept>

#include "Matt_daemon.hpp"
#include "Tintin_reporter.hpp"

int main() {
    std::unique_ptr<Matt_daemon> app;
    bool daemonized = false;

    try {
        app.reset(new Matt_daemon);

        app->daemonize(), daemonized = true;

        app->create_server();

        app->loop();
    } catch (const std::runtime_error &e) {
        if (!daemonized) {
            std::cerr << e.what() << std::endl;
        }
    }

    return 0;
}
