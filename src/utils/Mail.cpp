#include <filesystem>
#include <fstream>
#include "../header/Mail.h"

namespace fs = std::filesystem;

Mail::Mail() = default;
Mail::~Mail() = default;

Mail::Mail(string& sender_id, string &sender, string &receiver, string &subject, string &message, std::mutex* mutexMeta)
{
    this->sender_id = sender_id;
    this->sender = sender;
    this->receiver = receiver;
    this->subject = subject;
    this->message = message;
    this->mutexMeta = mutexMeta;
}

// creates the corresponding folders and meta file
// saves a message in sender, receiver folder
bool Mail::save(string& spool)
{
    string mail = 
                "--- sender ---\n" 
              + sender + "\n"
              + "--- receiver ---\n" 
              + receiver + "\n"
              + "--- subject ---\n" 
              + subject + "\n"
              + "--- message ---\n"
              + message + "\n";

    // meta-data file holds id of messages
    string metaSender = "._" + sender_id + "__.txt";
    string metaReceiver = "._" + receiver + "__.txt";
    bool messageToSelf = sender_id == receiver;
    try
    {
        // creates directory if not exists
        fs::create_directory(sender = spool + "/" + sender_id);
        fs::create_directory(receiver = spool + "/" + receiver);
    }
    catch(...)
    {
        return false;
    }

    mutexMeta->lock();
    // read out id of last saved mail and increment for sender
    ////////sender
    string s_number;    // message id sender

    {
        string file;
        string line;
        std::ifstream ReadNumber(spool + "/" + metaSender);
        getline(ReadNumber, line);
        int number = stoi(line) + 1;
        s_number = std::to_string(number);
        file.append(s_number + '\n');
        while(getline(ReadNumber,line))
        {
            file.append(line + '\n');
        }
        ReadNumber.close();
        
        std::ofstream MetaSenderFile( spool + "/" + metaSender, std::ofstream::trunc);
        MetaSenderFile << file;
        MetaSenderFile.close();
    }

    std::ofstream SendersFile(sender + "/" + s_number + "_" + subject + ".txt");
    SendersFile << mail;
    SendersFile.close();
    
    ////////receiver
    if(!messageToSelf)
    {
        string r_number;    // message id receiver
        if(!fs::exists(spool + "/" + metaReceiver))
        {
            std::ofstream MetaReceiverFile( spool + "/" + metaReceiver);
            MetaReceiverFile << "1";
            MetaReceiverFile.close();

            r_number = "1";
        }
        else
        {
            // read out id of last saved mail and increment
            string file;
            string line;
            std::ifstream ReadNumber(spool + "/" + metaReceiver);
            getline(ReadNumber, line);
            int number = stoi(line) + 1;
            r_number = std::to_string(number);
            file.append(r_number + '\n');

            while(getline(ReadNumber,line))
            {
                file.append(line + '\n');
            }
            ReadNumber.close();
            
            std::ofstream MetaReceiverFile( spool + "/" + metaReceiver, std::ofstream::trunc);
            MetaReceiverFile << file;
            MetaReceiverFile.close();
        }

        std::ofstream ReceiversFile(receiver + "/" + r_number + "_" + subject + ".txt");
        ReceiversFile << mail;
        ReceiversFile.close();
    }

    mutexMeta->unlock();
    return true;
}