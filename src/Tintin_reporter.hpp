#ifndef TINTIN_REPORTER_HPP
#define TINTIN_REPORTER_HPP

#include <fstream>
#include <iostream>
#include <memory>
#include <sys/stat.h>

class Tintin_reporter {
  public:
    enum Level { DEBUG, INFO, WARN, ERROR, CRITICAL, MAX };

    Tintin_reporter();
    Tintin_reporter(const Tintin_reporter &other);
    Tintin_reporter &operator=(const Tintin_reporter &other);

    void log(const char *message, Level level);
    void log_stdout(const char *message, Level level);

    ~Tintin_reporter();

  private:
    static constexpr char const *level_to_string[Level::MAX] = {"DEBUG", "INFO", "WARN", "ERROR",
                                                                "CRITICAL"};
    std::shared_ptr<std::ofstream> file;
};

#endif
