//
// manages output to console
//

#ifndef PRO_LOGGER_H
#define PRO_LOGGER_H

#include <string>

class Logger {
public:
    static void print_usage(char*);
    static void error(std::string);
    static void print(std::string);
};


#endif //PRO_LOGGER_H
