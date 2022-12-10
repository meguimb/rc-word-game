#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <word_image.h>

#define PORT "8080"
#define MAX_WORD_SIZE 64
#define MAX_ACTIVE_GAMES 10
#define WORD_FILENAME "word_file.txt"

typedef struct game {
    char player_plid[7];
    char *word;
    char curr_word_guess [MAX_WORD_SIZE];
    int max_errors;
    int trials;
} Game;

char *get_random_word_from_file(char filename []);
int receive_sng_command(char plid[7]);
int receive_plg_command(char plid[7], char letter, int trials);
int receive_pwg_command(char plid[7], char word_guess[MAX_WORD_SIZE], int trials);
int receive_qut_command(char plid[7]);
int get_max_errors(char *word);
int does_player_have_active_game(char plid [7]);

int fd, errcode;
ssize_t n;
socklen_t addrlen;
struct addrinfo hints, *res;
struct sockaddr_in addr;
char buffer[128];
Game *active_games [MAX_ACTIVE_GAMES];

int main(void){
    char PLID_TEMP[7], word[MAX_WORD_SIZE], letter;
    int trials;

    fd=socket(AF_INET,SOCK_DGRAM,0); //UDP socket
    if(fd==-1) /*error*/
        exit(1);
    memset(&hints,0,sizeof hints);
    hints.ai_family=AF_INET; // IPv4
    hints.ai_socktype=SOCK_DGRAM; // UDP socket
    hints.ai_flags=AI_PASSIVE;
    errcode=getaddrinfo(NULL,PORT,&hints,&res);
    if(errcode!=0) /*error*/ 
        exit(1);
    n=bind(fd,res->ai_addr, res->ai_addrlen);
    if(n==-1) /*error*/ 
        exit(1);
    while (1){
        addrlen=sizeof(addr);
        n=recvfrom(fd, buffer,128,0, (struct sockaddr*)&addr,&addrlen);
        if(n==-1)/*error*/
            exit(1);
        write(1,"received: ",10);
        write(1, buffer,n);
        n=sendto(fd,buffer,n,0, (struct sockaddr*)&addr,addrlen);
        if(n==-1)/*error*/
            exit(1);
        // check command coming from player.c
        if (sscanf(buffer, "SNG %6s\n", PLID_TEMP)){
            receive_sng_command(PLID_TEMP);
        }
        else if (sscanf(buffer, "PLG %s %c %c\n", PLID_TEMP, &letter, &trials)==3){
            receive_plg_command(PLID_TEMP, letter, trials-'0');
        }
        else if(sscanf(buffer, "PWG %s %s %d\n", PLID_TEMP, word, &trials)){
            receive_pwg_command(PLID_TEMP, word, trials);
        }
        else if(sscanf(buffer, "QUT %s\n", PLID_TEMP)){
            receive_qut_command(PLID_TEMP);
        }
    }
    freeaddrinfo(res);
    close(fd);
}  

int receive_sng_command(char plid[7]){
    int i, n;
    char response_buffer[64];
    printf("player number received from SNG is %s\n", plid);
    if (does_player_have_active_game(plid) == 1){
        printf("player has running game\n");
        // RSG NOK
        strcpy(response_buffer, "RSG NOK\n");
        n=sendto(fd, response_buffer, 8, 0, (struct sockaddr*)&addr,addrlen);
        if(n==-1)/*error*/
            exit(1);
        return 1;
    }

    printf("player does not have running game\n");
    // if status is OK then:
    for (i=0; i < MAX_ACTIVE_GAMES; i++){
        if (active_games[i] == NULL){
            active_games[i] = (Game *) malloc(sizeof(Game));
            active_games[i]->word = (char*) malloc(64*sizeof(char));
            strcpy(active_games[i]->player_plid, plid);
            // choose random word
            active_games[i]->word = get_random_word_from_file(WORD_FILENAME);
            printf("word for the player %s to guess is %s\n", active_games[i]->player_plid, active_games[i]->word);

            // set max error and number of trial
            active_games[i]->max_errors = get_max_errors(active_games[i]->word);
            active_games[i]->trials = 0;

            // set current word guess (all empty)
            for (int c = 0; c < strlen(active_games[i]->word); c++){
                active_games[i]->curr_word_guess[c] = '_';
            }
            // send stuff back to the player/client
            break;
        }
    }
    // send status OK
    // RSG status [n_letters max_errors]

    sprintf(response_buffer, "RSG OK %d %d\n", strlen(active_games[i]->word), active_games[i]->max_errors);
    printf("response buffer is %s", response_buffer);
    n=sendto(fd, response_buffer, 12, 0, (struct sockaddr*)&addr,addrlen);
    if(n==-1)/*error*/
        exit(1);
    // add player to text file or array
    return 0;
}

int receive_plg_command(char plid[7], char letter, int trials){
    printf("player %s is playing the letter %c and has %d trials left\n", plid, letter, trials);
    return 0;
}

int receive_pwg_command(char plid[7], char word_guess[MAX_WORD_SIZE], int trials){
    printf("player %s is guessing the word %s and has %d trials left\n", plid, word_guess, trials);
    return 0;
}

int receive_qut_command(char plid[7]){
    printf("quitting current game of user %s\n", plid);
    return 0;
}

char *get_random_word_from_file(char filename []){
    char *line, *word;
    int nOfLines;

    line = get_random_line(filename);
    word = get_word_from_line(line);
    free(line);
    return word;
}

int get_max_errors(char *word){
    int word_length, total_guesses;

    word_length = strlen(word);
    if (word_length <= 6){
        total_guesses = 7;
    }
    else if (word_length <= 10){
        total_guesses = 8;
    }
    else{
        total_guesses = 9;
    }
    return total_guesses;
}


int does_player_have_active_game(char plid [7]){
    char player_filename [16];
    sscanf(player_filename, "GAMES/GAME_%s.txt", plid);
    FILE *file;
    if ((file = fopen(player_filename, "r")))
    {
        fclose(file);
        return 1;
    }
    return 0;
}
/*
ficheiros em GAMES/GAME_123456.txt
palavra word_file.txt => palavra a decifrar + ficheiro de dicas
code(1) play(1)
code(2) play(2)

se code(n) é T (Trial) então play(n) é uma letra enviada pelo jogador
se code(n) é G (Guess) então play(n) é uma palavra enviada pelo jogador

NOTA: apenas jogadas novas são armazenadas, se a jogada for repetida, o server responde com
STATUS=DUP e não regista neste ficheiro

TÉRMINO DO JOGO
-> um jogo pode acabar com: sucesso, insucesso ou desistência
-> quando um jogo termina, o ficheiro GAME_123456.txt é transferido para a diretoria do jogador em GAMES/123456/ 
que é criada após o término do primeiro jogo deste player
-> nome do ficheiro muda => YYYYMMDD_HHMMSS_code.txt em que code é W(Win), F(Fail) ou Q(Quit)
*/