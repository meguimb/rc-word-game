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
    char *curr_word_guess;
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
int create_write_game_file(char plid [7], char word [], char filename []);
int find_player_game_index(char plid [7]);
int is_play_repeated(char plid [7], char letter);
int apply_letter_changes(char *curr_guess, char word [], char letter);

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
    if (does_player_have_active_game(plid) == 1){
        // RSG NOK
        strcpy(response_buffer, "RSG NOK\n");
        n=sendto(fd, response_buffer, 8, 0, (struct sockaddr*)&addr,addrlen);
        if(n==-1)/*error*/
            exit(1);
        return 1;
    }

    // if status is OK then:
    for (i=0; i < MAX_ACTIVE_GAMES; i++){
        if (active_games[i] == NULL){
            active_games[i] = (Game *) malloc(sizeof(Game));
            active_games[i]->word = (char*) malloc(64*sizeof(char));
            strcpy(active_games[i]->player_plid, plid);
            // choose random word
            active_games[i]->word = get_random_word_from_file(WORD_FILENAME);

            // set max error and number of trial
            active_games[i]->max_errors = get_max_errors(active_games[i]->word);
            active_games[i]->trials = 0;

            // set current word guess (all empty)
            active_games[i]->curr_word_guess = (char*) malloc(64*sizeof(char));
            for (int c = 0; c < strlen(active_games[i]->word); c++){
                active_games[i]->curr_word_guess[c] = '_';
            }
            active_games[i]->curr_word_guess[strlen(active_games[i]->word)] = '\0';
            // send stuff back to the player/client
            break;
        }
    }
    // create game file and write initial info
    create_write_game_file(active_games[i]->player_plid, active_games[i]->word, WORD_FILENAME);

    // RSG OK [n_letters max_errors]
    sprintf(response_buffer, "RSG OK %d %d\n", strlen(active_games[i]->word), active_games[i]->max_errors);
    n=sendto(fd, response_buffer, 12, 0, (struct sockaddr*)&addr,addrlen);
    if(n==-1)/*error*/
        exit(1);
    return 0;
}

int receive_plg_command(char plid[7], char letter, int trials){
    int index, changed;
    char response_buffer [9];
    printf("player %s is playing the letter %c and has %d trials left\n", plid, letter, trials);
    
    // find this player's game
    index = find_player_game_index(plid);
    printf("index is %d\n", index);
    if (index==-1){
        //  RLG ERR
        sprintf(response_buffer, "RLG ERR\n");
        n=sendto(fd, response_buffer, 9, 0, (struct sockaddr*)&addr,addrlen);
        if(n==-1)/*error*/
            exit(1);
        return 1;
    }
    
    // check number of guesses and number of trials
    if (active_games[index]->trials != trials){
        // RLG INV
        sprintf(response_buffer, "RLG INV\n");
        n=sendto(fd, response_buffer, 9, 0, (struct sockaddr*)&addr,addrlen);
        if(n==-1)/*error*/
            exit(1);
        return 1;
    }
    // check if this play is repeated
    if (is_play_repeated(active_games[index]->player_plid, letter)==1){
        //  RLG DUP
        sprintf(response_buffer, "RLG DUP\n");
        n=sendto(fd, response_buffer, 9, 0, (struct sockaddr*)&addr,addrlen);
        if(n==-1)/*error*/
            exit(1);
        return 1;
    }
    // do the play and change trials and curr_word_guess
    // RLG OK, NOK, WIN
    changed = apply_letter_changes(active_games[index]->curr_word_guess, active_games[index]->word, letter);
    printf("changed is %d\n", changed);
    if (changed==0){
        //  RLG NOK
        sprintf(response_buffer, "RLG NOK\n");
        n=sendto(fd, response_buffer, 9, 0, (struct sockaddr*)&addr,addrlen);
        if(n==-1)/*error*/
            exit(1);
    }
    else {
        if (strcmp(active_games[index]->curr_word_guess, active_games[index]->word)==0){
            //  RLG WIN
            sprintf(response_buffer, "RLG WIN\n");
            n=sendto(fd, response_buffer, 9, 0, (struct sockaddr*)&addr,addrlen);
            if(n==-1)/*error*/
                exit(1);
        }
        else {
            //  RLG OK
            sprintf(response_buffer, "RLG OK\n");
            printf("response buffer is %s", response_buffer);
            n=sendto(fd, response_buffer, 9, 0, (struct sockaddr*)&addr,addrlen);
            if(n==-1)/*error*/
                exit(1); 
        }
    }
    printf("word guess is now %s\n", active_games[index]->curr_word_guess);
    // update trials 
    // add play to file

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
    FILE *file;
    sprintf(player_filename, "GAMES/GAME_%s.txt", plid);
    if ((file = fopen(player_filename, "r")))
    {
        fclose(file);
        return 1;
    }
    return 0;
}

int create_write_game_file(char plid [7], char word [], char filename []){
    char buffer [128];
    FILE *fp;
    sprintf(buffer, "GAMES/GAME_%s.txt", plid);
    fp = fopen(buffer, "w");
    sprintf(buffer, "%s %s\n", word, filename);
    printf("buffer is %s", buffer);
    fputs(buffer, fp);
    fclose(fp);
    return 0;
}

int find_player_game_index(char plid [7]){
    int i;
    for (i=0; i < MAX_ACTIVE_GAMES; i++){
        if (active_games[i] != NULL && strcmp(active_games[i]->player_plid, plid)==0){
            return i;
        }
    }
    return -1;
}

int is_play_repeated(char plid [7], char letter){
    char filename[32], buffer [128], line[128];
    FILE *fp;
    sprintf(filename, "GAMES/GAME_%s.txt", plid);
    fp = fopen(filename, "r");
    while(fgets(line, 128, fp)) {
        sprintf(buffer, "T %c\n", letter);
        if (strcmp(buffer, line)==0){
            return 1;
        }
    }
    fclose(fp);
    return 0;
}

int apply_letter_changes(char *curr_guess, char word [], char letter){
    int i, in_word=0;
    for (i=0; i < strlen(word); i++){
        if (word[i] == letter){
            curr_guess[i] = letter;
            in_word = 1;
        }
    }
    return in_word;
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