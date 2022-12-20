#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <word_image.h>
#include <time.h>
#include <errno.h>

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
    int total_guesses;
} Game;

char *get_random_word_from_file(char filename []);
int receive_sng_command(char plid[7]);
int receive_plg_command(char plid[7], char letter, int trials);
int receive_pwg_command(char plid[7], char word_guess[MAX_WORD_SIZE], int trials);
int receive_qut_command(char plid[7]);
int receive_rev_command(char plid[7]);
int get_max_errors(char *word);
int does_player_have_active_game(char plid [7]);
int create_write_game_file(char plid [7], char word [], char filename []);
int find_player_game_index(char plid [7]);
int is_play_repeated(char plid [7], char letter);
int apply_letter_changes(char *curr_guess, char word [], char letter);
int write_letter_trial(char letter, char plid [7]);
int write_word_guess(char word_guess [], char plid [7]);
int send_rlg_ok_msg(int user_index, int letter_play);
int end_game_file_create(char plid [7], char code);
int delete_active_game(char plid [7]);
int score_file_create(char plid [7], struct tm *time);
int copy_text_file(char *source, char * destination);


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
        else if(sscanf(buffer, "REV %s\n", PLID_TEMP)){
            receive_rev_command(PLID_TEMP);
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
            active_games[i]->total_guesses = 0;

            // set current word guess (all empty)
            active_games[i]->curr_word_guess = (char*) malloc(64*sizeof(char));
            for (int c = 0; c < strlen(active_games[i]->word); c++){
                active_games[i]->curr_word_guess[c] = '_';
            }
            active_games[i]->word[strlen(active_games[i]->word)] = '\0';
            active_games[i]->curr_word_guess[strlen(active_games[i]->word)] = '\0';
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
    int index, changed = 0;
    char response_buffer [64], *buf;
    
    // find this player's game
    index = find_player_game_index(plid);
    if (index==-1){
        //  RLG ERR
        printf("RLG ERR\n");
        sprintf(response_buffer, "RLG ERR\n", trials);
        n=sendto(fd, response_buffer, 9, 0, (struct sockaddr*)&addr,addrlen);
        if(n==-1)/*error*/
            exit(1);
        return 1;
    }
    // check number of guesses and number of errors
    if (active_games[index]->trials != trials){
        // RLG INV
        printf("RLG INV\n");
        sprintf(response_buffer, "RLG INV %d\n", active_games[index]->trials);
        n=sendto(fd, response_buffer, 12, 0, (struct sockaddr*)&addr,addrlen);
        if(n==-1)/*error*/
            exit(1);
        return 1;
    }
    // check if this play is repeated
    if (is_play_repeated(active_games[index]->player_plid, letter)==1){
        //  RLG DUP
        printf("RLG DUP\n");
        sprintf(response_buffer, "RLG DUP %d\n", active_games[index]->trials);
        n=sendto(fd, response_buffer, 12, 0, (struct sockaddr*)&addr,addrlen);
        if(n==-1)/*error*/
            exit(1);
        return 1;
    }
    active_games[index]->total_guesses += 1;
    changed = apply_letter_changes(active_games[index]->curr_word_guess, active_games[index]->word, letter);
    if (changed==0){
        active_games[index]->trials += 1;
        
        // check if there are more errors left and if not RLG OVR
        if (active_games[index]->trials >= active_games[index]->max_errors){
            //  RLG OVR
            write_letter_trial(letter, plid);
            end_game_file_create(active_games[index]->player_plid, 'F');
            sprintf(response_buffer, "RLG OVR %d\n", active_games[index]->trials);
            n=sendto(fd, response_buffer, 12, 0, (struct sockaddr*)&addr,addrlen);
            if(n==-1)/*error*/
                exit(1);
            delete_active_game(plid);
        }
        //  RLG NOK
        else {
            write_letter_trial(letter, plid);
            sprintf(response_buffer, "RLG NOK %d\n", active_games[index]->trials);
            n=sendto(fd, response_buffer, 12, 0, (struct sockaddr*)&addr,addrlen);
            if(n==-1)/*error*/
                exit(1);
        }
    }
    else {
        if (strcmp(active_games[index]->curr_word_guess, active_games[index]->word)==0){
            //  RLG WIN
            write_letter_trial(letter, plid);
            end_game_file_create(active_games[index]->player_plid, 'W');
            sprintf(response_buffer, "RLG WIN %d\n",active_games[index]->trials);
            n=sendto(fd, response_buffer, 12, 0, (struct sockaddr*)&addr,addrlen);
            if(n==-1)/*error*/
                exit(1);
            delete_active_game(plid);
        }
        else {
            // RLG OK
            send_rlg_ok_msg(index, letter);
            write_letter_trial(letter, plid);
        }
    }
    return 0;
}

int receive_pwg_command(char plid[7], char word_guess[MAX_WORD_SIZE], int trials){
    int index;
    char response_buffer [16];
    index = find_player_game_index(plid);
    if (index==-1){
        // RLG ERR
        sprintf(response_buffer, "RWG ERR\n");
        n=sendto(fd, response_buffer, 8, 0, (struct sockaddr*)&addr,addrlen);
        if(n==-1)/*error*/
            exit(1); 
        return 1;
    }
    // INV
    if (active_games[index]->trials != trials){
        sprintf(response_buffer, "RWG INV %d\n", active_games[index]->trials);
        n=sendto(fd, response_buffer, 11, 0, (struct sockaddr*)&addr,addrlen);
        if(n==-1)/*error*/
            exit(1); 
        return 1;
    }
    // save play
    write_word_guess(word_guess, plid);
    printf("word_guess is %s\n", word_guess);
    active_games[index]->total_guesses += 1;
    if (strcmp(word_guess,active_games[index]->word)==0){
        end_game_file_create(active_games[index]->player_plid, 'W');
        sprintf(response_buffer, "RWG WIN %d\n", active_games[index]->trials);
        n=sendto(fd, response_buffer, 11, 0, (struct sockaddr*)&addr,addrlen);
        if(n==-1)/*error*/
            exit(1); 
        delete_active_game(plid);
    }
    // NOK and OVR
    else{
        active_games[index]->trials++;
        // OVR
        if (active_games[index]->max_errors <= active_games[index]->trials){
            end_game_file_create(active_games[index]->player_plid, 'F');
            sprintf(response_buffer, "RWG OVR %d\n", active_games[index]->trials);
            n=sendto(fd, response_buffer, 11, 0, (struct sockaddr*)&addr,addrlen);
            if(n==-1)/*error*/
                exit(1); 
            delete_active_game(plid);
        }
        else {
            sprintf(response_buffer, "RWG NOK %d\n", active_games[index]->trials);
            n=sendto(fd, response_buffer, 11, 0, (struct sockaddr*)&addr,addrlen);
            if(n==-1)/*error*/
                exit(1); 
        }
    }
    printf("end of ifs\n");
    return 0;
}

int receive_qut_command(char plid[7]){
    int index;
    char response_buffer[10];
    index = find_player_game_index(plid);
    if (index == -1){
        sprintf(response_buffer, "RQT ERR\n", active_games[index]->trials);
        n=sendto(fd, response_buffer, 8, 0, (struct sockaddr*)&addr,addrlen);
        if(n==-1)/*error*/
            exit(1); 
    }
    else{
        // end ongoing tcp connections
        end_game_file_create(active_games[index]->player_plid, 'Q');
        sprintf(response_buffer, "RQT OK\n", active_games[index]->trials);
        n=sendto(fd, response_buffer, 7, 0, (struct sockaddr*)&addr,addrlen);
        if(n==-1)/*error*/
            exit(1); 
    }
    return 0;
}

int receive_rev_command(char plid[7]){
    /*
    RRV word/status
    During project development in reply to a REV request the GS server sends a
    back the word to be guessed.
    For the final version of the project this command should result in game
    termination, with the GS server sending a status message. If player PLID had
    an ongoing game, the GS replies with status = OK, after closing any open
    TCP connections with this Player. Otherwise the status is ERR.
    */
    int index;
    char response_buffer[64];
    printf("beginning of rev receiving command\n");
    index = find_player_game_index(plid);
    if (index==-1){
        sprintf(response_buffer, "RRV ERR\n");
        n=sendto(fd, response_buffer, 8, 0, (struct sockaddr*)&addr,addrlen);
        if(n==-1)/*error*/
            exit(1); 
    }
    else {
        printf("rrv, index is %d and word is %s\n", index, active_games[index]->word);
        // TODO: closing TCP connections
        sprintf(response_buffer, "RRV %s/OK\n", active_games[index]->word);
        n=sendto(fd, response_buffer, 9+strlen(active_games[index]->word), 0, (struct sockaddr*)&addr,addrlen);
        if(n==-1)/*error*/
            exit(1); 
        // TODO: terminate game
    }
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

int write_letter_trial(char letter, char plid [7]){
    char filename[32], buffer [128];
    FILE *fp;
    sprintf(filename, "GAMES/GAME_%s.txt", plid);
    sprintf(buffer, "T %c\n", letter);
    fp = fopen(filename, "a");
    fputs(buffer, fp);
    fclose(fp);
    return 0;
}


int write_word_guess(char word_guess [], char plid [7]){
    char filename[32], buffer [128];
    FILE *fp;
    sprintf(filename, "GAMES/GAME_%s.txt", plid);
    sprintf(buffer, "G %s\n", word_guess);
    fp = fopen(filename, "a");
    fputs(buffer, fp);
    fclose(fp);
    return 0;
}

int send_rlg_ok_msg(int user_index, int letter_play){
    char *buf = (char*) malloc(128*sizeof(char)), *buf_ptr = buf;
    int pos [strlen(active_games[user_index]->word)];
    int n = 0;
    for (int i = 0; i < strlen(active_games[user_index]->word); i++){
        if (active_games[user_index]->word[i] == letter_play){
            pos[n] = i;
            n++;
        }
    }
    pos[n] = '\0';
    buf_ptr += sprintf(buf_ptr, "RLG OK %d %d", active_games[user_index]->trials, n);
    for (int i = 0 ; i != n ; i++) {
        buf_ptr += sprintf(buf_ptr, " %d", pos[i]);
    }
    buf_ptr += sprintf(buf_ptr, "\n");
    write(1, buf, 12+n*2);
    n=sendto(fd, buf, 12+n*2, 0, (struct sockaddr*)&addr,addrlen);
    //n=sendto(fd, response_buffer, 14+(n*4), 0, (struct sockaddr*)&addr,addrlen);
    if(n==-1)/*error*/
        exit(1); 
    free(buf);
    return 0;
}

int end_game_file_create(char plid [7], char code){
    int ret;
    FILE *file;
    char oldname [64], newname [64];
    time_t now = time(NULL);
    struct tm *t = gmtime(&now);
    sprintf(oldname, "GAMES/GAME_%s.txt", plid);
    sprintf(newname, "GAMES/%s/", plid);
    // create dir for user
    mkdir(newname, S_IRWXU);
    sprintf(newname, "GAMES/%s/%d%d%d_%02d%02d%02d_%c.txt", plid, t->tm_year+1900, t->tm_mon+1, 
        t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec, code);
    copy_text_file(oldname, newname);
    if (code == 'W'){
        score_file_create(plid, t);
    }
    remove(oldname);
    return 0;
}

int score_file_create(char plid [7], struct tm *time){
    int index, right_guesses, score;
    FILE *fp;
    Game *game;
    char filename [64];

    index = find_player_game_index(plid);
    game = active_games[index];

    right_guesses = game->total_guesses - game->trials;
    score = 100 * right_guesses / game->total_guesses;

    sprintf(filename, "SCORES/%03d_%s_%d%d%d_%02d%02d%02d.txt", score, plid, time->tm_year+1900, time->tm_mon+1, 
        time->tm_mday, time->tm_hour, time->tm_min, time->tm_sec);

    sprintf(buffer, "%d %s %s %d %d", score, plid, game->word, right_guesses, game->total_guesses);
    fp = fopen(filename, "a");
    fputs(buffer, fp);
    fclose(fp);
    return 0;
}

int delete_active_game(char plid [7]){
    int index; Game *player_game;
    index = find_player_game_index(plid);
    player_game = active_games[index];
    free(player_game->curr_word_guess);
    free(player_game->word);
    free(player_game);
    // free curr_word_guess and word
    active_games[index] = NULL;
    return 0;
}

int copy_text_file(char *source, char * destination){
    char ch;
    FILE *source_file, *destination_file;
    source_file = fopen(source, "r");
    destination_file = fopen(destination, "w");
    while ((ch = fgetc(source_file)) != EOF)
        fputc(ch, destination_file);
    fclose(source_file);
    fclose(destination_file);
    return 0;
}