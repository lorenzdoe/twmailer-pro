//
// handles serverside communication with client
//

#ifndef PRO_COMMHANDLER_H
#define PRO_COMMHANDLER_H

#include <string>
#include <mutex>
#include "globals.h"

using std::string;

struct PersonData
{
    string uid;
    string name;
};

class CommHandler {
private:
    int socket;
    char buffer[BUF];
    string message;         //client communication variable
    string spool;           //mail spool directory

    // extensions for pro version
    string ip_address;
    struct PersonData personData;
    bool loggedIn;
    int loginCount;
    std::mutex* mutexBlack; //Blacklist mutex
    std::mutex* mutexMeta;  //meta file mutex

    bool handle();
    bool send_client();
    bool receive_client();

    // protocolls
    bool send_protocol();
    bool list_protocol();
    bool read_protocol();
    bool delete_protocol();
    bool login_protocol();

    // predefined messages to client
    void OK();
    void ERR();

    // helpers
    void incrementLoginCounts(); // creates meta file if not exists, reads out failed login attempts
    bool ipBlacklisted();        // returns true if ip is on the blacklist
    void blacklistIP();          // writes user-ip in blacklist if login-counts >= 3
    void resetLoginCounts();     // sets login counts to 0 in blacklist meta

public:
    CommHandler();
    CommHandler(int socket, string ip_address, string spool,  std::mutex* mutexBlack, std::mutex* mutexMeta);
    ~CommHandler();

    // client communication
    void clientCommunication(int socket);
};


#endif //PRO_COMMHANDLER_H
