//
// defines the Server class
//
#ifndef PRO_SERVER_H
#define PRO_SERVER_H

#include <netinet/in.h>
#include <iostream>
#include <string>
#include <list>
#include <thread>
#include <mutex>
#include <vector>

#include "../header/CommHandler.h"

namespace twMailer
{
    using std::string;
}

using namespace twMailer;

class Server {
private:
    int port;
    socklen_t addrlen;
    struct sockaddr_in address, cliaddress;
    int reuseValue = 1;
    string spool;

    // pro
    std::vector<std::thread> threadPool;
    std::mutex mutexBlack;
    std::mutex mutexMeta;

public:
    Server(int port,string spool);
    ~Server();
    static void signalHandler(int);
    static void closeSockets();
    void start();
    void run();
};


#endif //PRO_SERVER_H
