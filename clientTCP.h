#ifndef CLIENT_TCP_H
#define CLIENT_TCP_H
extern int fd, errcode;
extern ssize_t n;
extern socklen_t addrlen;
extern struct addrinfo hints, *res;
extern struct sockaddr_in addr;
extern char buffer[128];

void connect_to_server();
void scoreboard();
void hint(char* PLID);
void state(char* PLID);
#endif