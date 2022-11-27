//
// Created by lorenz on 24.11.22.
//

#ifndef PRO_GLOBALS_H
#define PRO_GLOBALS_H

#define BUF 1024

////////////////////////// have to be global due to signal handler
extern bool abortRequested;
extern int create_socket;         //
extern int new_socket;

#endif //PRO_GLOBALS_H