#include "Tintin_reporter.hpp"

Tintin_reporter::Tintin_reporter() : file{std::make_shared<std::ofstream>()} {
    if (mkdir("/var/log/matt_daemon", 0775) == -1) {
        if (errno != EEXIST) {
            throw std::runtime_error(
                "/var/log/matt_daemon directory does not exists and cannot be created");
        }
    }

    file->open("/var/log/matt_daemon/matt_daemon.log", std::ofstream::app);

    if (!file->is_open()) {
        throw std::runtime_error("Log file cannot be created");
    }
}

Tintin_reporter::Tintin_reporter(const Tintin_reporter &other) { file = other.file; }

Tintin_reporter &Tintin_reporter::operator=(const Tintin_reporter &other) {
    file = other.file;

    return *this;
}

void Tintin_reporter::log(const char *message, Level level) {
    auto now = std::chrono::system_clock::now();
    std::time_t currentTime = std::chrono::system_clock::to_time_t(now);
    std::tm *localTime = std::localtime(&currentTime);

    *file << "[" << std::setw(2) << std::setfill('0') << localTime->tm_mday << "/" << std::setw(2)
          << std::setfill('0') << (localTime->tm_mon + 1) << "/" << std::setw(4)
          << (localTime->tm_year + 1900) << "-" << std::setw(2) << std::setfill('0')
          << localTime->tm_hour << ":" << std::setw(2) << std::setfill('0') << localTime->tm_min
          << ":" << std::setw(2) << std::setfill('0') << localTime->tm_sec << "]"
          << " [ " << Tintin_reporter::level_to_string[level] << " ] - Matt_daemon: " << message
          << std::endl;
}

void Tintin_reporter::log_stdout(const char *message, Level level) {
    auto now = std::chrono::system_clock::now();
    std::time_t currentTime = std::chrono::system_clock::to_time_t(now);
    std::tm *localTime = std::localtime(&currentTime);

    std::cout << "[" << std::setw(2) << std::setfill('0') << localTime->tm_mday << "/"
              << std::setw(2) << std::setfill('0') << (localTime->tm_mon + 1) << "/" << std::setw(4)
              << (localTime->tm_year + 1900) << "-" << std::setw(2) << std::setfill('0')
              << localTime->tm_hour << ":" << std::setw(2) << std::setfill('0') << localTime->tm_min
              << ":" << std::setw(2) << std::setfill('0') << localTime->tm_sec << "]"
              << " [ " << Tintin_reporter::level_to_string[level] << " ] - Matt_daemon: " << message
              << std::endl;
}

Tintin_reporter::~Tintin_reporter() { file->flush(); }
