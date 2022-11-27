TW mailer PRO - Project for course "Distributed Systems"
FH Technikum Wien, WS 2022
Creators: Victor Bartosik and Lorenz Döllinger 


Socket programming in C, C++
Client-Server architecture

Server (mailserver):
login clients using LDAP
saves messages of users in spooldir
usage: ./program-name <PORT-NR> <spooldir-name>

Client:
login to server
send messages
list, read, delete sent messages
usage: ./program-name <server-ip> <PORT-NR> 

Spooldir:
spooldir saves messages in folders named after users and contains metadata for all users
metadata structure:
- first line = mesage id
followed by alternating:
- ip address
- number of bad login attempts coming from this ip address
  (ip adress of client is blacklisted for 1 minute after 3 bad attempts
    and attempts are reseted to 0)
