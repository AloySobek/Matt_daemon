#include "Tintin_reporter.hpp"

Logger::Logger(std::shared_ptr<std::ostream> output, std::string name)
    : output{output}, name{name}, level{Logger::Logger::Level::DEBUG} {}

Logger::Logger(const Logger &other) {
    if (&other != this) {
        name = other.name;
        level = other.level;
        output = other.output;
    }
}

Logger &Logger::operator=(const Logger &other) {
    if (&other != this) {
        name = other.name;
        level = other.level;
        output = other.output;
    }

    return *this;
}

void Logger::set_level(Level level) { this->level = level; }

void Logger::debug(const char *message) {
    if (level <= Logger::Level::DEBUG) {
        log(message, Logger::Level::DEBUG);
    }
}

void Logger::info(const char *message) {
    if (level <= Logger::Level::INFO) {
        log(message, Logger::Level::INFO);
    }
}

void Logger::warn(const char *message) {
    if (level <= Logger::Level::WARN) {
        log(message, Logger::Level::WARN);
    }
}

void Logger::error(const char *message) {
    if (level <= Logger::Level::ERROR) {
        log(message, Logger::Level::ERROR);
    }
}

void Logger::critical(const char *message) {
    if (level <= Logger::Level::CRITICAL) {
        log(message, Logger::Level::CRITICAL);
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
            << " [ " << Logger::level_to_string[level] << " ] - " << name << ": " << message
            << std::endl;
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
