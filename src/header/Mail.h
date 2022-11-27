/*
 * this class processes a mail
 */
#ifndef BASIC_MAIL_H
#define BASIC_MAIL_H

#include <string>
#include <mutex>

using std::string;

class Mail
{
private:
    string sender_id;
    string sender;
    string receiver;
    string subject;
    string message;

    std::mutex* mutexMeta;

public:
    Mail();
    Mail(string& sender_id, string& sender, string& receiver, string& subject, string& message, std::mutex* mutexMeta);
    ~Mail();

    bool save(string& spool);
};


#endif //BASIC_MAIL_H
