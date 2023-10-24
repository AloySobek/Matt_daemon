#include <iostream>
#include <stdexcept>

#include "Matt_daemon.hpp"
#include "Tintin_reporter.hpp"

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
