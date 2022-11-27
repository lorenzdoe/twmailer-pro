/*
    read in pw from terminal
*/
#ifndef PRO_GETPW_H
#define PRO_GETPW_H

#include <termios.h>
#include <unistd.h>
#include <string>

using std::string;

int getch();
string getpw();

#endif