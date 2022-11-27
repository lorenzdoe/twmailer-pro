//
//  tw-mailer pro server-main
//
#include <string>
#include <filesystem>
#include <fstream>

#include "../header/Logger.h"
#include "../header/Server.h"

namespace twMailer
{
    namespace fs = std::filesystem;
}

int main(int argc, char* argv[])
{
    int port;
    std::string spool;

    if(argc < 3)
    {
        Logger::error("error: no port or Mail-spool-directoryname passed");
        Logger::print_usage(argv[0]);
        exit(EXIT_FAILURE);
    }

    port = atoi(argv[1]);       //converts char* to int
    spool = argv[2];                //saves spool directory name
    fs::create_directory(spool);
    std::ofstream blacklist(spool + "/.BLACKLIST.txt", std::ios_base::app);
    blacklist.close();

    auto* server = new Server(port, spool);
    server->start();

    delete server;
}