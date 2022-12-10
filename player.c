#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#define PORT "8080"

int do_start_command(char plid [7]);
int do_play_command(char letter);
int do_guess_command(char word [64]);
int do_scoreboard_command();
int do_hint_command();
int do_state_command();
int do_quit_command();
int do_exit_command();

int fd, errcode, trials, ongoing_game=0, playing=1;
ssize_t n;
socklen_t addrlen;
struct addrinfo hints, *res;
struct sockaddr_in addr;
char PLID[7];

int main(){
    char letter, input[16], word[64], PLID_TEMP[7];
    while (playing==1){
        fgets(input, sizeof input, stdin);
        if (sscanf(input, "start %6s", PLID_TEMP) || sscanf(input, "sg %6s", PLID_TEMP)){
            do_start_command(PLID_TEMP);
        }
        else if (sscanf(input, "play %c", &letter) || sscanf(input, "pl %c", &letter)){
            do_play_command(letter);
        }
        else if (sscanf(input, "guess %s", word) || sscanf(input, "gw %s", word)){
            do_guess_command(word);
        }
        else if (strcmp(input, "scoreboard\n")==0 || strcmp(input, "sb\n")==0){
            do_scoreboard_command();
        }
        else if (strcmp(input, "hint\n")==0 || strcmp(input, "h\n")==0){
            do_hint_command();
        }
        else if (strcmp(input, "state\n")==0 || strcmp(input, "st\n")==0){
            do_state_command();
        }
        else if (strcmp(input, "quit\n")==0){
            do_quit_command();
        }
        else if (strcmp(input, "exit\n")==0){
            do_exit_command();
        }
        else{
            printf("you fucked up\n");
        }
    }

    return 0;
}

int do_start_command(char plid [7]){
    int trials, word_length;
    char buffer[128], message[128];
    if (ongoing_game == 1){
        return 1;
    }
    ongoing_game = 1;
    strcpy(PLID, plid);
    fd=socket(AF_INET,SOCK_DGRAM,0);
    if(fd==-1){
        exit(1);
    }
    memset(&hints,0,sizeof hints);

    hints.ai_family=AF_INET; //IPv4
    hints.ai_socktype=SOCK_DGRAM; //UDP socket

    errcode=getaddrinfo("margab-manjaro", PORT, &hints,&res);
    if(errcode!=0){
        exit(1);
    }
    sprintf(message, "SNG %s\n", plid);
    n = sendto(fd, message, 11, 0, res->ai_addr, res->ai_addrlen);
    if(n==-1){
        exit(1);
    }
    addrlen=sizeof(addr);
    n = recvfrom(fd, buffer,128,0, (struct sockaddr*)&addr,&addrlen);
    if(n==-1) {
        exit(1);
    }
    write(1,"echo: ", 6); 
    write(1, message, n);
    n=recvfrom(fd, buffer,128,0, (struct sockaddr*)&addr,&addrlen);
    if(n==-1)/*error*/
        exit(1);
    // check status from server
    if (sscanf(buffer, "RSG NOK\n")){
        printf("NOK\n");
    }
    else if (sscanf(buffer, "RSG OK %d %d\n", &word_length, &trials)){
        printf("OK, length of word is %d and number of trials is %d\n", word_length, trials);
    }
    // sends message to Game Server using UDP to start a new game
    return 0;
}

int do_play_command(char letter){
    printf("you chose to play the letter %c\n", letter);
    char buffer[128], message[128];
    sprintf(message, "PLG %s %c %c\n", PLID, letter, trials + '0');
    n = sendto(fd, message, 15 , 0, res->ai_addr, res->ai_addrlen);
    if(n==-1){
        exit(1);
    }
    addrlen=sizeof(addr);
    n = recvfrom(fd, buffer,128,0, (struct sockaddr*)&addr,&addrlen);
    if(n==-1) {
        exit(1);
    }
    write(1,"echo: ", 6); 
    write(1, message, n);
    // sends message to Game Server using UDP to start a new game
    return 0;
}

int do_guess_command(char word [64]){
    char buffer[128], message[128];
    printf("you tried to guess the word %s\n", word);
    sprintf(message, "PWG %s %s %d\n", PLID, word, trials);
    n = sendto(fd, message, 14+strlen(word), 0, res->ai_addr, res->ai_addrlen);
    if(n==-1){
        exit(1);
    }
    addrlen=sizeof(addr);
    n = recvfrom(fd, buffer,128,0, (struct sockaddr*)&addr,&addrlen);
    if(n==-1) {
        exit(1);
    }
    write(1,"echo: ", 6); 
    write(1, message, n);
    return 0;
}

int do_scoreboard_command(){
    printf("_|___|___|_\n");
    printf("_|___|___|_\n");
    printf(" |   |   | \n");
    return 0;
}

int do_hint_command(){
    printf("the hint is: potato\n");
    return 0;
}

int do_state_command(){
    printf("get state: winner or loser\n");
    return 0;
}

int do_quit_command(){
    char buffer[128], message[128];
    printf("quitting the game...\n");
    sprintf(message, "QUT %s\n", PLID);
    n = sendto(fd, message, 11, 0, res->ai_addr, res->ai_addrlen);
    if(n==-1){
        exit(1);
    }
    addrlen=sizeof(addr);
    n = recvfrom(fd, buffer,128,0, (struct sockaddr*)&addr,&addrlen);
    if(n==-1) {
        exit(1);
    }
    write(1,"echo: ", 6); 
    write(1, message, n);
    return 0;
}

int do_exit_command(){
    printf("exiting the game...\n");
    return 0;
}