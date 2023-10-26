#include "Tintin_reporter.hpp"

Logger::Logger(std::shared_ptr<std::ostream> output, std::string name)
    : output{output}, name{name}, level{Level::DEBUG} {}

Logger::Logger(const Logger &other) {
    if (&other != this) {
        output = other.output;
    }
}

Logger &Logger::operator=(const Logger &other) {
    if (&other != this) {
        output = other.output;
    }

    return *this;
}

void Logger::set_level(Level level) { this->level = level; }

void Logger::debug(const char *message) {
    if (level <= Level::DEBUG) {
        log(message, Level::DEBUG);
    }
}

void Logger::info(const char *message) {
    if (level <= Level::INFO) {
        log(message, Level::INFO);
    }
}

void Logger::warn(const char *message) {
    if (level <= Level::WARN) {
        log(message, Level::WARN);
    }
}

void Logger::error(const char *message) {
    if (level <= Level::ERROR) {
        log(message, Level::ERROR);
    }
}

void Logger::critical(const char *message) {
    if (level <= Level::CRITICAL) {
        log(message, Level::CRITICAL);
    }
}

Logger::~Logger() { output->flush(); }

void Logger::log(const char *message, Level level) {
    auto now = std::chrono::system_clock::now();
    std::time_t currentTime = std::chrono::system_clock::to_time_t(now);
    std::tm *localTime = std::localtime(&currentTime);

    *output << "[" << std::setw(2) << std::setfill('0') << localTime->tm_mday << "/" << std::setw(2)
            << std::setfill('0') << (localTime->tm_mon + 1) << "/" << std::setw(4)
            << (localTime->tm_year + 1900) << "-" << std::setw(2) << std::setfill('0')
            << localTime->tm_hour << ":" << std::setw(2) << std::setfill('0') << localTime->tm_min
            << ":" << std::setw(2) << std::setfill('0') << localTime->tm_sec << "]"
            << " [ " << level_to_string[level] << " ] - " << name << ": " << message << std::endl;
}

Tintin_reporter::Tintin_reporter(std::shared_ptr<std::ofstream> file, std::string name)
    : Logger(file, name) {}

Tintin_reporter::Tintin_reporter(const Tintin_reporter &other) : Logger(other) {}

Tintin_reporter &Tintin_reporter::operator=(const Tintin_reporter &other) {
    if (&other != this) {
        Logger::operator=(other);
    }

    return *this;
}

Tintin_reporter::~Tintin_reporter() {}
