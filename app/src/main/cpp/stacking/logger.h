//
// Created by sedv2 on 17.04.2026.
//

#ifndef STACKING_LOGGER_H
#define STACKING_LOGGER_H

#include <core/logger.h>
#include <cstdint>

class JniLogger : public ILogger {
private:
    static const size_t LOG_BUFFER_SIZE = 512;
    static char LOG_BUFFER[LOG_BUFFER_SIZE];
public:
    void log(bool isError, const char *format, ...) override;
};

#endif //STACKING_LOGGER_H
