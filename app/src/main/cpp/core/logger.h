//
// Created by sedv2 on 17.04.2026.
//

#ifndef CORE_LOGGER_H
#define CORE_LOGGER_H

class ILogger {
public:
    virtual ~ILogger() = default;
    virtual void log(bool isError, const char *format, ...) = 0;
};

#endif //CORE_LOGGER_H
