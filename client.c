#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <signal.h>

#include "clientTCP.h"
//58011
#define PORT "8080"

int fd, errcode;
ssize_t n;
socklen_t addrlen;
struct addrinfo hints, *res;
struct sockaddr_in addr;
char buffer[128];
int trials;
char game_word[30];

void start(char* PLID) {
    char *msg = malloc(11);
    char status[3];
    int n_letters, n_errors;
    sprintf(msg, "SNG %s\n", PLID);
    n=sendto(fd, msg, 11, 0, res->ai_addr, res->ai_addrlen);
    if (n == -1) {exit(1);}
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
            game_word[i] = '_';
            printf(" _");
        }
        printf("\n");
    }
}

void play(char* PLID, char letter) {
    char *msg = malloc(15);
    char status[3];
    int occ, t;
    int o = 0;
    sprintf(msg, "PLG %s %c %d\n", PLID, letter, trials);
    if (trials>9) {n=sendto(fd, msg, 16, 0, res->ai_addr, res->ai_addrlen);}
    else {n=sendto(fd, msg, 15, 0, res->ai_addr, res->ai_addrlen);}
    if (n == -1) {exit(1);}
    free(msg);
    addrlen = sizeof(addr);
    n=recvfrom(fd, buffer, 128, 0, (struct sockaddr*) &addr, &addrlen);
    if (n == -1) {exit(1);}
    sscanf(buffer, "RLG %s %d", status, &t);
    if(strcmp(status, "OK") == 0) {
        sscanf(buffer, "RLG %s %d %d", status, &t, &occ);
        if(trials>9) {o=1;};
        for(int i = 0; i < occ; i++) {
            if(buffer[12+i*2+o] == ' ' || buffer[12+i*2+o] == '\n') {
                game_word[(buffer[11+i*2+o] - '0')-1] = letter;
            }
            else {
                game_word[10*(buffer[11+i*2+o] - '0')+(buffer[12+i*2+o] -'0')-1] = letter;
                o++;
            }
        }
        trials++;
        printf("Word:");
        for(int i = 0; i < strlen(game_word); i++) {
            printf(" %c", game_word[i]);
        }
        printf("\n");
    }
    else if(strcmp(status, "NOK") == 0) {
        trials++;
        printf("Letter not in word.\n");
    }
    else if(strcmp(status, "WIN") == 0) {printf("You won!\n");}
    else if(strcmp(status, "DUP") == 0) {printf("Letter already guessed.\n");}
    else if(strcmp(status, "OVR") == 0) {printf("Game over.\n");}
    else if(strcmp(status, "INV") == 0) {printf("Invalid number of trials.\n");}
    else{printf("Error.\n");}
}

void guess(char* PLID, char* word, int size) {
    char *msg = malloc(14+size);
    char status[3];
    sprintf(msg, "PWG %s %s %d\n", PLID, word, trials);
    if (trials>9) {n=sendto(fd, msg, 15+size, 0, res->ai_addr, res->ai_addrlen);}
    else {n=sendto(fd, msg, 14+size, 0, res->ai_addr, res->ai_addrlen);}
    if (n == -1) {exit(1);}
    free(msg);
    addrlen = sizeof(addr);
    n=recvfrom(fd, buffer, 128, 0, (struct sockaddr*) &addr, &addrlen);
    if (n == -1) {exit(1);}
    sscanf(buffer, "RWG %s", status);
    if(strcmp(status, "NOK") == 0) {
        printf("Wrong guess.\n");
        trials++;
    }
    else if(strcmp(status, "WIN") == 0) {printf("You won!\n");}
    else if(strcmp(status, "NOK") == 0) {printf("Wrong guess.\n");}
    else if(strcmp(status, "OVR") == 0) {printf("Game over.\n");}
    else if(strcmp(status, "INV") == 0) {printf("Invalid number of trials.\n");}
    else{printf("Error.\n");}
}

void quit(char* PLID) {
    char *msg = malloc(11);
    char status[3];
    for(int i = 0; i < 60; i++) {
        game_word[i] = '\0';
    }
    sprintf(msg, "QUT %s\n", PLID);
    n=sendto(fd, msg, 11, 0, res->ai_addr, res->ai_addrlen);
    if (n == -1) {exit(1);}
    free(msg);
    addrlen = sizeof(addr);
    n=recvfrom(fd, buffer, 128, 0, (struct sockaddr*) &addr, &addrlen);
    if (n == -1) {exit(1);}
    sscanf(buffer, "RQT %s", status);
    if(strcmp(status, "OK") == 0) {printf("Game ended.\n");}
}

void rev(char* PLID) {
    char *msg = malloc(11);
    char word[30];
    sprintf(msg, "REV %s\n", PLID);
    n = sendto(fd, msg, 11, 0, res->ai_addr, res->ai_addrlen);
    if(n==-1){exit(1);}
    free(msg);
    addrlen=sizeof(addr);
    n = recvfrom(fd, buffer,128,0, (struct sockaddr*)&addr,&addrlen);
    if(n==-1) {exit(1);}
    sscanf(buffer, "RRV %s\n", word);
    printf("%s\n", word);
}
//tejo.tecnico.ulisboa.pt
//LAPTOP-PUAA9FRQ
int main() {
    char letter, input[128], word[30], PLID[7];
    pid_t child;
    int returnStatus;
    fd=socket(AF_INET, SOCK_DGRAM, 0);
    if (fd == -1) {exit(1);}
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    errcode = getaddrinfo("LAPTOP-PUAA9FRQ", PORT, &hints, &res);
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
            int size = strlen(word);
            guess(PLID, word, size);
        }
        else if (strcmp(input, "scoreboard\n")==0 || strcmp(input, "sb\n")==0){
            if ((child = fork()) == 0) {
                scoreboard();
                exit(0);
            }
            waitpid(child, &returnStatus, 0);
        }
        else if (strcmp(input, "hint\n")==0 || strcmp(input, "h\n")==0){
            if ((child = fork()) == 0) {
                hint(PLID);
                exit(0);
            }
            waitpid(child, &returnStatus, 0);
        }
        else if (strcmp(input, "state\n")==0 || strcmp(input, "st\n")==0){
            if ((child = fork()) == 0) {
                state(PLID);
                exit(0);
            }
            waitpid(child, &returnStatus, 0);
        }
        else if (strcmp(input, "quit\n")==0){
            quit(PLID);
        }
        else if (strcmp(input, "exit\n")==0){
            quit(PLID);
            break;
        }
        else if (strcmp(input, "rev\n")==0){
            rev(PLID);
        }
        else{
            printf("you fucked up\n");
        }
        memset(buffer, 0, sizeof buffer);
    }
    return 0;
}
