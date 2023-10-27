#ifndef TINTIN_REPORTER_HPP
#define TINTIN_REPORTER_HPP

#include <chrono>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <sys/stat.h>

// Not thread safe!
class Logger {
  public:
    enum Level { DEBUG, INFO, WARN, ERROR, CRITICAL, MAX };

    Logger(std::shared_ptr<std::ostream> output, std::string name);
    Logger(const Logger &other);
    Logger &operator=(const Logger &other);

    void set_level(Level level);

    void debug(const char *message);
    void info(const char *message);
    void warn(const char *message);
    void error(const char *message);
    void critical(const char *message);

    ~Logger();

  private:
    static constexpr char const *level_to_string[Level::MAX] = {"DEBUG", "INFO", "WARN", "ERROR",
                                                                "CRITICAL"};
    std::shared_ptr<std::ostream> output;
    std::string name;
    Level level;

    void log(const char *message, Level level);
};

// Tintin_reporter is file only logger
class Tintin_reporter : public Logger {
  public:
    Tintin_reporter(std::shared_ptr<std::ofstream> file, std::string name);
    Tintin_reporter(const Tintin_reporter &other);
    Tintin_reporter &operator=(const Tintin_reporter &other);

    ~Tintin_reporter();
};

#endif
