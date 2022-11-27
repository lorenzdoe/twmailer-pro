/*
 * tw-mailer pro
 * 
 * SERVER CLASS
*/
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <csignal>
#include <cstring>
#include <iostream>
#include <string>
#include <utility>
#include <filesystem>

#include "../header/Server.h"
#include "../header/Logger.h"

bool abortRequested = false;
int create_socket = -1;
int new_socket = -1;

namespace twMailer
{
    using std::cerr;
    using std::endl;
    using std::cout;
    namespace fs = std::filesystem;
}

Server::Server(int port, string spool)
{
    this->port = port;
    this->spool = std::move(spool);
}

Server::~Server() = default;

void Server::signalHandler(int sig)
{
    if( sig == SIGINT )
    {
        Logger::print("abort Requested...");
        abortRequested = true;

        // With shutdown() one can initiate normal TCP close sequence ignoring
        // the reference count.
        if(new_socket != -1)
        {
            if(shutdown(new_socket, SHUT_RDWR) == -1)
            {
                Logger::error("shutdown new_socket");
            }
            if(close(new_socket) == -1)
            {
                Logger::error("close new_socket");
            }
        }

        if(create_socket != -1)
        {
            if(shutdown(create_socket, SHUT_RDWR) == -1)
            {
                Logger::error("shutdown create_socket");
            }
            if(close(create_socket) == -1)
            {
                Logger::error("close create_socket");
            }
            create_socket = -1;
        }
    }
    else
    {
        exit(sig);
    }
}

void Server::start()
{

    ////////////////////////////////////////////////////////////////////////////
    // SIGNAL HANDLER
    // SIGINT (Interrup: ctrl+c) is registered in for handling
    // https://man7.org/linux/man-pages/man2/signal.2.html
    if( signal(SIGINT, Server::signalHandler) == SIG_ERR)
    {
        Logger::error("error: signal can not be registered");
        exit(EXIT_FAILURE);
    }

    ////////////////////////////////////////////////////////////////////////////
    // CREATE A SOCKET
    // https://man7.org/linux/man-pages/man2/socket.2.html
    // https://man7.org/linux/man-pages/man7/ip.7.html
    // https://man7.org/linux/man-pages/man7/tcp.7.html
    // IPv4, TCP (connection oriented), IP (same as client)
    if((create_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1 )
    {
        Logger::error("Socket error");
        exit(EXIT_FAILURE);
    }

    ////////////////////////////////////////////////////////////////////////////
    // SET SOCKET OPTIONS
    // https://man7.org/linux/man-pages/man2/setsockopt.2.html
    // https://man7.org/linux/man-pages/man7/socket.7.html
    // socket, level, optname, optvalue, optlen
    if(setsockopt(create_socket,
                  SOL_SOCKET,
                  SO_REUSEADDR,
                  &reuseValue,
                  sizeof(reuseValue)) == -1)
    {
        cerr << "set socket options - reuseAddr" << endl;
        exit(EXIT_FAILURE);
    }

    if(setsockopt(create_socket,
                  SOL_SOCKET,
                  SO_REUSEPORT,
                  &reuseValue,
                  sizeof(reuseValue)) == -1)
    {
        Logger::error("set socket options - reusePort");
        exit(EXIT_FAILURE);
    }

    ////////////////////////////////////////////////////////////////////////////
    // INIT ADDRESS
    // Attention: network byte order => big endian
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET; // IPv4
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(this->port);

    ////////////////////////////////////////////////////////////////////////////
    // ASSIGN AN ADDRESS WITH PORT TO SOCKET
    if( bind(create_socket, (struct sockaddr *)&address, sizeof(address)) == -1 )
    {
        Logger::error("bind error");
        exit(EXIT_FAILURE);
    }

    ////////////////////////////////////////////////////////////////////////////
    // ALLOW CONNECTION ESTABLISHING
    // Socket, Backlog (= count of waiting connections allowed)
    if( listen(create_socket, 5) == -1)
    {
        Logger::error("listen error");
        exit(EXIT_FAILURE);
    }


    this->run();

}

void Server::run()
{

    while(!abortRequested)
    {
        Logger::print("Waiting for connections...");

        /////////////////////////////////////////////////////////////////////////
        // ACCEPTS CONNECTION SETUP
        // blocking, might have an accept-error on ctrl+c
        addrlen = sizeof(struct sockaddr_in);

        if ( (new_socket = accept(create_socket,(struct sockaddr *)&cliaddress, &addrlen)) == -1 )
        {
            if(abortRequested)
            {
                Logger::error("accept error after aborted");
            }
            else
            {
                Logger::error("accept error");
            }
            break;
        }

        string cli_ip_addr = inet_ntoa(cliaddress.sin_addr);
        /////////////////////////////////////////////////////////////////////////
        // START CLIENT
        // ignore printf error handling
        cout << "Client connected from "
             << cli_ip_addr
             << ": " << ntohs(cliaddress.sin_port)
             << endl;


        CommHandler commHandler(new_socket, cli_ip_addr, spool, &mutexBlack, &mutexMeta);
        //Communications.push_back(commHandler);
        threadPool.emplace_back(&CommHandler::clientCommunication, commHandler, new_socket);

    }
    for(auto& th: threadPool)
    {
        th.join();
    }
    /*
    for(auto& handler: Communications)
    {
        delete handler;
    }
    Communications.clear();
     */

    new_socket = -1;
    closeSockets();
}

void Server::closeSockets()
{

    if(new_socket != -1)
    {
        if(shutdown(new_socket, SHUT_RDWR) == -1)
        {
            Logger::error("shutdown new_socket");
        }
        if(close(new_socket) == -1)
        {
            Logger::error("close new_socket");
        }
    }

    if(create_socket != -1)
    {
        if(shutdown(create_socket, SHUT_RDWR) == -1)
        {
            Logger::error("shutdown create_socket");
        }
        if(close(create_socket) == -1)
        {
            Logger::error("close create_socket");
        }
        create_socket = -1;
    }
}