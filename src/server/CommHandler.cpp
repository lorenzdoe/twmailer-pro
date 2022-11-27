//
// tw-mailer pro
//
// COMMUNICATION HANDLER
//
#include <unistd.h>
#include <netinet/in.h>
#include <ldap.h>
#include <iostream>
#include <string>
#include <filesystem>
#include <fstream>
#include <utility>

#include "../header/CommHandler.h"
#include "../header/Mail.h"
#include "../header/Logger.h"

namespace twMailer
{
    using std::cerr;
    using std::cout;
    using std::endl;
    namespace fs = std::filesystem;
}

using namespace twMailer;

using std::string;

CommHandler::CommHandler() = default;
CommHandler::~CommHandler() = default;

CommHandler::CommHandler(int socket, string ip_address, string spool, std::mutex* mutexBlack, std::mutex* mutexMeta)
{
    this->socket = socket;
    this->ip_address = std::move(ip_address);
    this->spool = std::move(spool);
    this->loggedIn = false;
    this->loginCount = 0;
    this->mutexBlack = mutexBlack;
    this->mutexMeta = mutexMeta;
}

void CommHandler::clientCommunication(int socket)
{
    ////////////////////////////////////////////////////////////////////////////
    // SEND welcome message
    message = "Welcome to myserver!\r\nPlease enter your commands...\r\n";
    if( !send_client() )
    {
        return;
    }

    ////// main loop where client communication takes place
    do
    {

        if(!handle())
        {
            break;
        }

    } while( !abortRequested );

    // closes/frees the descriptor if not already
    if (socket != -1)
    {
        if (shutdown(socket, SHUT_RDWR) == -1)
        {
            perror("shutdown new_socket");
        }
        if (close(socket) == -1)
        {
            perror("close new_socket");
        }
        socket = -1;
    }
}

bool CommHandler::handle() {
    if( !receive_client())
    {
        return false;
    }

    // get option from message
    string option = message.substr(0,message.find('\n')); //option is always first line
    message = message.substr(message.find('\n')+1,message.length());

    if(option == "QUIT")
    {
        return false;
    }

    else if(!loggedIn)
    {
        if(option == "LOGIN")
        {
            //TODO:: check if blacklisted
            if(ipBlacklisted())
            {
                ERR();
                return true;
            }

            if (!login_protocol())
            {
                // read out failed login attempts
                // create meta file if not exists

                incrementLoginCounts();
                if(loginCount >= 3) {
                    blacklistIP();    // checks if ip should be blacklisted 
                }
                ERR();
            } 
            else
            {
                resetLoginCounts();
                OK();
            }
        } 
        else  // if not loggedin and option is something else than login
        {
            ERR();
        }
        return true;
    }

    if(option == "SEND")
    {
        if(!send_protocol())
        {
            ERR();
        }
        else
        {
            OK();
        }
        return true;
    }
    else if(option == "LIST")
    {
        if(!list_protocol())
        {
            ERR();
        }
        else
        {
            return send_client();
        }
        return true;
    }
    else if(option == "READ")
    {
        if(!read_protocol())
        {
            ERR();
        }
        else
        {
            return send_client();
        }
        return true;
    }
    else if(option == "DEL")
    {
        if(delete_protocol())
        {
            OK();
        }
        else
        {
            ERR();
        }
        return true;
    }
    else
    {
        message = "invalid command";
        return send_client();
    }
}

bool CommHandler::send_client()
{
    if(send(socket, message.c_str(), message.length(), 0) == -1)
    {
        cerr << "send answer failed" << endl;
        return false;
    }
    return true;
}

bool CommHandler::receive_client()
{
    int size;
    /////////////////////////////////////////////////////////////////////////
    // RECEIVE
    size = recv(socket, buffer, BUF - 1 , 0);
    if( size == -1 /* just to be sure */)
    {
        if(abortRequested)
        {
            cerr << "recv error after aborted " << endl;
        }
        else
        {
            cerr << "recv error" << endl;
        }
        return false;
    }

    if( size == 0 )
    {
        cout << "Client closed remote socket" << endl;
        return false;
    }

    // remove ugly debug message, because of the sent newline of client
    if (buffer[size - 2] == '\r' && buffer[size - 1] == '\n')
    {
        size -= 2;
    }
    else if (buffer[size - 1] == '\n')
    {
        --size;
    }
    buffer[size] = '\0';

    message = buffer;   // handles buffer in string message

    return true;
}

bool CommHandler::send_protocol()
{
    //ÜBERARBEITEN
    string sender = this->personData.name;

    string receiver = message.substr(0,message.find('\n'));
    message = message.substr(message.find('\n')+1, message.length());

    if(receiver.empty() || receiver.length() > 8 || receiver.find('/') != string::npos)
        return false;

    // not allow sending message to self
    if(sender == receiver)
        return false;

    string subject = message.substr(0,message.find('\n'));
    message = message.substr(message.find('\n')+1, message.length());

    if(subject.empty() || subject.length() > 80)
        return false;

    // not allow subject containing . or /
    if(subject.find('.') != string::npos || subject.find('/') != string::npos)
        return false;

    Mail mail(this->personData.uid, sender, receiver, subject, message, mutexMeta);

    return mail.save(spool);
}

bool CommHandler::list_protocol()
{
    string output;
    int count = 0;
    for(const auto& dirEntry: fs::directory_iterator(spool + "/" + personData.uid))
    {
        // cuts .txt from filename
        count++;
        string filename = dirEntry.path().filename();
        filename = filename.substr(0, filename.length()-4);
        output += filename + '\n';
    }
    output = "messages: " + std::to_string(count) + '\n' + output;
    message = output;
    return true;
}

bool CommHandler::read_protocol()
{
    if(message.empty())
    {
        return false;
    }
    else
    {
        string msgNumber = message.substr(0,message.length());

        if(msgNumber.empty() || !fs::exists(spool + "/" + personData.uid) || !fs::is_directory(spool + "/" + personData.uid))
        {
            // user unkwn | bad input
            return false;
        }
        else
        {
            for(const auto& dirEntry: fs::directory_iterator(spool + "/" + personData.uid))
            {
                string compare = dirEntry.path().filename();
                compare = compare.substr(0, compare.find('_'));
                if(msgNumber == compare)
                {
                    std::ifstream Message((string)dirEntry.path());
                    string collect;
                    message = "OK\n";
                    while(getline(Message, collect))
                    {
                        message.append(collect + '\n');
                    }
                    Message.close();
                    return true;
                }
            }
        }
    }
    return false;
}

bool CommHandler::delete_protocol()
{
    if(message.empty())
    {
        return false;
    }
    else
    {
        string msgNumber = message.substr(0, message.length());

        if(msgNumber.empty() || !fs::exists(spool + "/" + personData.uid) || !fs::is_directory(spool + "/" + personData.uid))
        {
            // user unkwn | bad input
            return false;
        }
        else
        {
            for(const auto& dirEntry: fs::directory_iterator(spool + "/" + personData.uid))
            {
                string compare = dirEntry.path().filename();
                compare = compare.substr(0, compare.find('_'));
                if(msgNumber == compare)
                {
                    return fs::remove(dirEntry.path());
                }
            }
        }
    }
    return false;
}

bool CommHandler::login_protocol()
{
    ////////////////////////////////////////////////////////////////////////////
    // LDAP config
    // anonymous bind with user and pw empty
    const char *ldapUri = "ldap://ldap.technikum-wien.at:389";
    const int ldapVersion = LDAP_VERSION3;

    string uname = message.substr(0,message.find('\n'));
    string pwd = message.substr(message.find('\n')+1, message.length());
    string binduname = "uid=" + uname + ",ou=people,dc=technikum-wien,dc=at";

    // move temp try uid to class instance
    // needed for blacklisting
    this->personData.uid = uname;

    // needed for ldap protocoll
    const char* username = binduname.c_str();
    const char* password = pwd.c_str();;

    // general
    int rc = 0; // return code

    ////////////////////////////////////////////////////////////////////////////
    // setup LDAP connection
    // https://linux.die.net/man/3/ldap_initialize
    LDAP *ldapHandle;
    rc = ldap_initialize(&ldapHandle, ldapUri);
    if (rc != LDAP_SUCCESS)
    {
        fprintf(stderr, "ldap_init failed\n");
        return false;
    }
    printf("connected to LDAP server %s\n", ldapUri);

    ////////////////////////////////////////////////////////////////////////////
    // set verison options
    // https://linux.die.net/man/3/ldap_set_option
    rc = ldap_set_option(
        ldapHandle,
        LDAP_OPT_PROTOCOL_VERSION, // OPTION
        &ldapVersion);             // IN-Value
    if (rc != LDAP_OPT_SUCCESS)
    {
        // https://www.openldap.org/software/man.cgi?query=ldap_err2string&sektion=3&apropos=0&manpath=OpenLDAP+2.4-Release
        fprintf(stderr, "ldap_set_option(PROTOCOL_VERSION): %s\n", ldap_err2string(rc));
        ldap_unbind_ext_s(ldapHandle, NULL, NULL);
        return false;
    }

    ////////////////////////////////////////////////////////////////////////////
    // start connection secure (initialize TLS)
    // https://linux.die.net/man/3/ldap_start_tls_s
    // int ldap_start_tls_s(LDAP *ld,
    //                      LDAPControl **serverctrls,
    //                      LDAPControl **clientctrls);
    // https://linux.die.net/man/3/ldap
    // https://docs.oracle.com/cd/E19957-01/817-6707/controls.html
    //    The LDAPv3, as documented in RFC 2251 - Lightweight Directory Access
    //    Protocol (v3) (http://www.faqs.org/rfcs/rfc2251.html), allows clients
    //    and servers to use controls as a mechanism for extending an LDAP
    //    operation. A control is a way to specify additional information as
    //    part of a request and a response. For example, a client can send a
    //    control to a server as part of a search request to indicate that the
    //    server should sort the search results before sending the results back
    //    to the client.
    rc = ldap_start_tls_s(
        ldapHandle,
        NULL,
        NULL);
    if (rc != LDAP_SUCCESS)
    {
        fprintf(stderr, "ldap_start_tls_s(): %s\n", ldap_err2string(rc));
        ldap_unbind_ext_s(ldapHandle, NULL, NULL);
        return false;
    }

    ////////////////////////////////////////////////////////////////////////////
    // bind credentials
    // https://linux.die.net/man/3/lber-types
    // SASL (Simple Authentication and Security Layer)
    // https://linux.die.net/man/3/ldap_sasl_bind_s
    // int ldap_sasl_bind_s(
    //       LDAP *ld,
    //       const char *dn,
    //       const char *mechanism,
    //       struct berval *cred,
    //       LDAPControl *sctrls[],
    //       LDAPControl *cctrls[],
    //       struct berval **servercredp);

    BerValue bindCredentials;
    bindCredentials.bv_val = (char *)password;
    bindCredentials.bv_len = strlen(password);
    BerValue *servercredp; // server's credentials
    rc = ldap_sasl_bind_s(
        ldapHandle,
        username,
        LDAP_SASL_SIMPLE,
        &bindCredentials,
        NULL,
        NULL,
        &servercredp);
    if (rc != LDAP_SUCCESS)
    {
        fprintf(stderr, "LDAP bind error: %s\n", ldap_err2string(rc));
        ldap_unbind_ext_s(ldapHandle, NULL, NULL);
        return false;
    }


    // search settings
    const char *ldapSearchBaseDomainComponent = "dc=technikum-wien,dc=at";
    const char *ldapSearchFilter = ("(uid=" + uname + ")").c_str();
    ber_int_t ldapSearchScope = LDAP_SCOPE_SUBTREE;
    const char *ldapSearchResultAttributes[] = {"cn", NULL};

    ////////////////////////////////////////////////////////////////////////////
    // perform ldap search
    // https://linux.die.net/man/3/ldap_search_ext_s
    // _s : synchronous
    // int ldap_search_ext_s(
    //     LDAP *ld,
    //     char *base,
    //     int scope,v
    //     char *filter,
    //     char *attrs[],
    //     int attrsonly,
    //     LDAPControl **serverctrls,
    //     LDAPControl **clientctrls,
    //     struct timeval *timeout,
    //     int sizelimit,
    //     LDAPMessage **res );
    LDAPMessage *searchResult;
    rc = ldap_search_ext_s(
        ldapHandle,
        ldapSearchBaseDomainComponent,
        ldapSearchScope,
        ldapSearchFilter,
        (char **)ldapSearchResultAttributes,
        0,
        NULL,
        NULL,
        NULL,
        500,
        &searchResult);
    if (rc != LDAP_SUCCESS)
    {
        fprintf(stderr, "LDAP search error: %s\n", ldap_err2string(rc));
        ldap_unbind_ext_s(ldapHandle, NULL, NULL);
        return false;
    }
    // https://linux.die.net/man/3/ldap_count_entries
    ldap_count_entries(ldapHandle, searchResult);

    ////////////////////////////////////////////////////////////////////////////
    // get result of search
    // https://linux.die.net/man/3/ldap_first_entry
    // https://linux.die.net/man/3/ldap_next_entry
    LDAPMessage *searchResultEntry;
    for (searchResultEntry = ldap_first_entry(ldapHandle, searchResult);
            searchResultEntry != NULL;
            searchResultEntry = ldap_next_entry(ldapHandle, searchResultEntry))
    {
        /////////////////////////////////////////////////////////////////////////
        // Base Information of the search result entry
        // https://linux.die.net/man/3/ldap_get_dn
        ldap_get_dn(ldapHandle, searchResultEntry);

        /////////////////////////////////////////////////////////////////////////
        // Attributes
        // https://linux.die.net/man/3/ldap_first_attribute
        // https://linux.die.net/man/3/ldap_next_attribute
        //
        // berptr: berptr, a pointer to a BerElement it has allocated to keep
        //         track of its current position. This pointer should be passed
        //         to subsequent calls to ldap_next_attribute() and is used to
        //         effectively step through the entry's attributes.
        BerElement *ber;
        char *searchResultEntryAttribute;
        for (searchResultEntryAttribute = ldap_first_attribute(ldapHandle, searchResultEntry, &ber);
            searchResultEntryAttribute != NULL;
            searchResultEntryAttribute = ldap_next_attribute(ldapHandle, searchResultEntry, ber))
        {
            BerValue **vals;
            if ((vals = ldap_get_values_len(ldapHandle, searchResultEntry, searchResultEntryAttribute)) != NULL)
            {

                personData.name = vals[0]->bv_val;
                ldap_value_free_len(vals);
            }

            // free memory
            ldap_memfree(searchResultEntryAttribute);
        }
        // free memory
        if (ber != NULL)
        {
            ber_free(ber, 0);
        }
    }

    // free memory
    ldap_msgfree(searchResult);

    ////////////////////////////////////////////////////////////////////////////
    // https://linux.die.net/man/3/ldap_unbind_ext_s
    // int ldap_unbind_ext_s(
    //       LDAP *ld,
    //       LDAPControl *sctrls[],
    //       LDAPControl *cctrls[]);
    ldap_unbind_ext_s(ldapHandle, NULL, NULL);

    this->personData.uid = std::move(uname);
    this->loggedIn = true;

    return true;
}

void CommHandler::OK()
{
    message = "OK";
    send_client();
}

void CommHandler::ERR()
{
    message = "ERR";
    send_client();
}

///////////////////////////// helpers

void CommHandler::incrementLoginCounts()
{
    this->loginCount++;
    
    mutexMeta->lock();
    string metaFile = "._" + this->personData.uid + "__.txt";
    // if no meta file exists create one
    if(!fs::exists(spool + "/" + metaFile))
    {
        std::ofstream MetaSenderFile( spool + "/" + metaFile);
        MetaSenderFile << "0\n" // message id
                        + ip_address + '\n'
                        + '1';
        MetaSenderFile.close();
    } 
    else  // if metafile already exists
    {
    // search for ip, read out attempts, increment (+ write in metaFile) and save in class
    // if ip not in meta then save ip + 0 (attempts)
        std::ifstream ReadCounts(spool + "/" + metaFile);
        string line;
        string file;
        bool found = false;
        while(getline(ReadCounts, line))
        {
            if(line.find(this->ip_address) != string::npos){
                found = true;
                file.append(line + '\n');
                getline(ReadCounts, line);
                this->loginCount = stoi(line) + 1;
                line = std::to_string(loginCount);
            }
            file.append(line + '\n');
        }
        if(!found)  // create new entry with ip
        {
            file.append(ip_address + "\n1");
        }
        std::ofstream writeCounts(spool + "/" + metaFile, std::ios_base::trunc);
        writeCounts << file;
        writeCounts.close();
        ReadCounts.close();
    }
    mutexMeta->unlock();
}

bool CommHandler::ipBlacklisted()
{
    // read timestamp from blacklist
    std::ifstream readBlacklist(spool + "/.BLACKLIST.txt");
    string line;
    long long exp = -1;
    while(getline(readBlacklist, line))
    {
        if(line.find(ip_address) != string::npos)
        {
            getline(readBlacklist, line);
            exp = stoll(line);
        }
    }
    readBlacklist.close();

    const auto n = std::chrono::system_clock::now();
    const auto now = std::chrono::duration_cast<std::chrono::seconds>(n.time_since_epoch()).count();

    return now <= exp;
}

void CommHandler::blacklistIP()
{
    // reset user login counts (starts at 0 again)
    // search in blacklist for ip and overwrite expirationtime
    // if ip not in blacklist - append blacklist
    resetLoginCounts();

    mutexBlack->lock();
    const auto now = std::chrono::system_clock::now();
    const auto exp = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count() + 60;

    std::ifstream readBlacklist(spool + "/.BLACKLIST.txt");
    string file;
    string line;
    bool foundIP = false;
    while(getline(readBlacklist, line))
    {
        if(line.find(ip_address) != string::npos)
        {
            foundIP = true;
            file.append(line + '\n');
            getline(readBlacklist, line);
            line = std::to_string(exp) + '\n';
        }
        file.append(line + '\n');
    }

    if(!foundIP)
    {
        file.append(ip_address + '\n' + std::to_string(exp) + '\n');
    }
    readBlacklist.close();

    // write in blacklist    
    std::ofstream writeBlacklist(spool + "/.BLACKLIST.txt", std::ios_base::trunc);
    writeBlacklist << file;
    writeBlacklist.close();

    mutexBlack->unlock();
}

void CommHandler::resetLoginCounts()
{
    // search for ip, read out attempts, increment (+ write in metaFile) and save in class
    // if ip not in meta then save ip + 0 (attempts)
    mutexBlack->lock();

    this->loginCount = 0;
    string path = spool + "/._" + personData.uid + "__.txt";
    string file;

    if(fs::exists(path))
    {
        std::ifstream ReadMetaFIle(path);
        string line;
        bool found = false;
        while(getline(ReadMetaFIle, line))
        {
            if(line.find(this->ip_address) != string::npos){
                found = true;
                file.append(line + '\n');
                getline(ReadMetaFIle, line);
                line = '0';
            }
            file.append(line + '\n');
        }
        if(!found)  // create new entry with ip
        {
            file.append(ip_address + "\n0");
        }
        ReadMetaFIle.close();
    }
    else
    {
        file.append("0\n" + ip_address + "\n0");
    }
    std::ofstream writeMetaFile(path, std::ios_base::trunc);
    writeMetaFile << file;
    writeMetaFile.close();

    mutexBlack->unlock();
}