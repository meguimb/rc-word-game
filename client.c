#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <stdio.h>
#define PORT "58011"

int fd, errcode;
ssize_t n;
socklen_t addrlen;
struct addrinfo hints, *res;
struct sockaddr_in addr;
char buffer[128];
int trials;

int mai(void) {

    fd=socket(AF_INET, SOCK_DGRAM, 0);
    if (fd == -1) {
        exit(1);
    }
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;

    errcode = getaddrinfo("tejo.tecnico.ulisboa.pt", PORT, &hints, &res);
    if (errcode != 0) {
        exit(1);
    }
    
    n=sendto(fd, "SNG 123456\n", 11, 0, res->ai_addr, res->ai_addrlen);
    if (n == -1) {
        exit(1);
    }

    addrlen = sizeof(addr);
    n=recvfrom(fd, buffer, 128, 0, (struct sockaddr*) &addr, &addrlen);
    if (n == -1) {
        exit(1);
    }

    write(1, "echo: ", 10);
    write(1, buffer, n);

    freeaddrinfo(res);
    close(fd);
}

void start(char* PLID) {
    char *msg = malloc(11);
    char status[3];
    int n_letters, n_errors;
    sprintf(msg, "SNG %s\n", PLID);
    n=sendto(fd, msg, 11, 0, res->ai_addr, res->ai_addrlen);
    if (n == -1) {
        exit(1);
    }
    free(msg);
    addrlen = sizeof(addr);
    n=recvfrom(fd, buffer, 128, 0, (struct sockaddr*) &addr, &addrlen);
    if (n == -1) {
        exit(1);
    }
    sscanf(buffer, "RSG %s %d %d", status, &n_letters, &n_errors);
    if(strcmp(status, "OK") == 0) {
        trials = 1;
        printf("New game started. Guess %d letter word:", n_letters);
        for(int i = 0; i < n_letters; i++) {
            printf(" _");
        }
        printf("\n");
    }
}

void play(char* PLID, char letter) {
    char *msg = malloc(15);
    char status[3];
    int occ, t;
    char pos[10];
    sprintf(msg, "PLG %s %c %d\n", PLID, letter, trials);
    n=sendto(fd, msg, 15, 0, res->ai_addr, res->ai_addrlen);
    if (n == -1) {
        exit(1);
    }
    free(msg);
    addrlen = sizeof(addr);
    n=recvfrom(fd, buffer, 128, 0, (struct sockaddr*) &addr, &addrlen);
    if (n == -1) {
        exit(1);
    }
    sscanf(buffer, "RLG %s %d", status, &t);
    if(strcmp(status, "OK") == 0) {
        trials++;
        sscanf(buffer, "RLG %s %d %d", status, &t, &occ);
        printf("Letter %c occurs %d times in positions %s\n", letter, occ, pos);
    }

}

void quit(char* PLID) {
    char *msg = malloc(11);
    char status[3];
    sprintf(msg, "QUT %s\n", PLID);
    n=sendto(fd, msg, 11, 0, res->ai_addr, res->ai_addrlen);
    if (n == -1) {
        exit(1);
    }
    free(msg);
    addrlen = sizeof(addr);
    n=recvfrom(fd, buffer, 128, 0, (struct sockaddr*) &addr, &addrlen);
    if (n == -1) {
        exit(1);
    }
    sscanf(buffer, "RQT %s", status);
    if(strcmp(status, "OK") == 0) {
        printf("Game ended.\n");
    }
}

int main(){
    char letter, input[16], word[30], PLID[7];

    fd=socket(AF_INET, SOCK_DGRAM, 0);
    if (fd == -1) {exit(1);}
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    errcode = getaddrinfo("tejo.tecnico.ulisboa.pt", PORT, &hints, &res);
    if (errcode != 0) {exit(1);}
    while(1) {
        printf("Enter a command: ");
        fgets(input, sizeof input, stdin);
        if (sscanf(input, "start %6s", PLID) || sscanf(input, "sg %6s", PLID)){
            start(PLID);
        }
        else if (sscanf(input, "play %c", &letter) || sscanf(input, "pl %c", &letter)){
            play(PLID, letter);
        }
        else if (sscanf(input, "guess %s", word) || sscanf(input, "gw %s", word)){
        }
        else if (strcmp(input, "scoreboard\n")==0 || strcmp(input, "sb\n")==0){
        }
        else if (strcmp(input, "hint\n")==0 || strcmp(input, "h\n")==0){
        }
        else if (strcmp(input, "state\n")==0 || strcmp(input, "st\n")==0){
        }
        else if (strcmp(input, "quit\n")==0){
            quit(PLID);
        }
        else if (strcmp(input, "exit\n")==0){
            quit(PLID);
            break;
        }
        else{
            printf("you fucked up\n");
        }
        memset(buffer, 0, sizeof buffer);
    }

    return 0;
}