//
// Created by lorenz on 18.11.22.
//
#include <iostream>
#include <string>

#include "../header/Logger.h"

namespace twMailer
{
    using std::cout;
    using std::cerr;
    using std::endl;
    using std::string;
}
using namespace twMailer;

void Logger::print_usage(char* program_name)
{
    cout << "Usage: " << program_name << " <port> <Mail-spool-directoryname>" << endl;
}

void Logger::error(string s)
{
    cerr << s << endl;
}

void Logger::print(string s)
{
    cout << s << endl;
}