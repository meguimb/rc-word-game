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

#define DEFAULT_PORT "8080"
#define MAX_WORD_SIZE 30
#define MAX_FILENAME_SIZE 64
#define MAX_PORT_SIZE 6
#define MAX_ACTIVE_GAMES 10

int receive_cmd_arguments(int argc, char *argv []);
int make_udp_connection();
char *get_random_word(char filename []);
int get_max_errors(char *word);
int find_player_game_index(char *PLID);
int is_play_repeated(char plid [7], char letter);
int apply_letter_changes(char *curr_guess, char word [], char letter);
int write_letter_trial(char letter, char plid [7]);
int write_word_guess(char word_guess [], char plid [7]);
int copy_text_file(char *source, char * destination);
int score_file_create(char plid [7], struct tm *time);
int end_game_file_create(char plid [7], char code);
int delete_active_game(char plid [7]);
int do_rlg_ok_msg(int user_index, int letter_play);
int create_active_game(int index, char plid[7]);
int do_sng(char* PLID);
int do_rsg(char status [], int index);
int do_rlg(char status [4], int index);
int do_plg(char *PLID, char letter, int trials);
int do_pwg(char* PLID, char* word, int trials);
int do_qut(char* PLID);
int do_rev(char *PLID);

typedef struct game {
    char player_plid[7];
    char *word;
    char *curr_word_guess;
    int max_errors;
    int errors;
    int trials;
} Game;

int fd, errcode;
ssize_t n;
socklen_t addrlen;
struct addrinfo hints, *res;
struct sockaddr_in addr;
char buffer[128];
Game *active_games [MAX_ACTIVE_GAMES];
char word_filename [MAX_FILENAME_SIZE];
char port [MAX_PORT_SIZE];


int main(int argc, char *argv[]){
    char C_PLID[7], word[MAX_WORD_SIZE], letter;
    int trials;

    receive_cmd_arguments(argc, argv);
    make_udp_connection();

    while (1){
        addrlen=sizeof(addr);
        n=recvfrom(fd, buffer, 128, 0, (struct sockaddr*)&addr,&addrlen);
        if(n==-1) exit(1);
        write(1,"received: ", 10);
        write(1, buffer, n);
        if (sscanf(buffer, "SNG %s\n", C_PLID)) {
            do_sng(C_PLID);
        }
        else if (sscanf(buffer, "PLG %s %c %d\n", C_PLID, &letter, &trials)){
            do_plg(C_PLID, letter, trials);
        }
        else if(sscanf(buffer, "PWG %s %s %d\n", C_PLID, word, &trials)){
            do_pwg(C_PLID, word, trials);
        }
        else if(sscanf(buffer, "QUT %s\n", C_PLID)){
            do_qut(C_PLID);
        }
        else if(sscanf(buffer, "REV %s\n", C_PLID)){
            do_rev(C_PLID);
        }
    }
    freeaddrinfo(res);
    close(fd);
    return 0;
}

char *get_random_word(char filename []){
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
    if (word_length <= 6) {total_guesses = 7;}
    else if (word_length <= 10) {total_guesses = 8;}
    else {total_guesses = 9;}
    return total_guesses;
}

int find_player_game_index(char *PLID){
    int i;
    for (i=0; i < MAX_ACTIVE_GAMES; i++){
        if (active_games[i] != NULL && strcmp(active_games[i]->player_plid, PLID)==0){return i;}
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

int score_file_create(char plid [7], struct tm *time){
    int index, right_guesses, score;
    FILE *fp;
    Game *game;
    char filename [64];

    index = find_player_game_index(plid);
    game = active_games[index];

    right_guesses = game->trials - game->errors;
    score = 100 * right_guesses / game->trials;

    sprintf(filename, "SCORES/%03d_%s_%d%d%d_%02d%02d%02d.txt", score, plid, time->tm_year+1900, time->tm_mon+1, 
        time->tm_mday, time->tm_hour, time->tm_min, time->tm_sec);

    sprintf(buffer, "%d %s %s %d %d", score, plid, game->word, right_guesses, game->trials);
    fp = fopen(filename, "a");
    fputs(buffer, fp);
    fclose(fp);
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

int do_rlg_ok_msg(int user_index, int letter_play){
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
    if(n==-1) exit(1); 
    free(buf);
    return 0;
}


int do_sng(char* PLID) {
    int n, i;
    char buffer[128], player_filename[16];
    FILE *fp;
    sprintf(player_filename, "GAMES/GAME_%s.txt", PLID);
    if ((fp = fopen(player_filename, "r"))) {
        fclose(fp);
        do_rsg("NOK", -1);
    }
    else {
        for (i = 0; i < MAX_ACTIVE_GAMES; i++) {
            if (active_games[i] == NULL) {
                break;
            }
        }
        create_active_game(i, PLID);
        fp = fopen(player_filename, "w");
        sprintf(player_filename, "%s %s\n", active_games[i]->word, word_filename);
        fputs(player_filename, fp);
        fclose(fp);
        do_rsg("OK", i);
    }
    return 0;
}

int do_plg(char *PLID, char letter, int trials) {
    int index, changed = 0;
    char buffer[128];
    index = find_player_game_index(PLID);
    if (index==-1){
        do_rlg("ERR", index);
    }
    else if (active_games[index]->trials + 1 != trials) {
        do_rlg("INV", index);
    }
    else if (is_play_repeated(active_games[index]->player_plid, letter)==1){
        do_rlg("DUP", index);
    }
    else {
        changed = apply_letter_changes(active_games[index]->curr_word_guess, active_games[index]->word, letter);
        active_games[index]->trials += 1;
        if (changed==0){
            active_games[index]->errors += 1;
            if (active_games[index]->errors >= active_games[index]->max_errors){
                write_letter_trial(letter, PLID);
                end_game_file_create(active_games[index]->player_plid, 'F');
                do_rlg("OVR", index);
                delete_active_game(PLID);
            }
            else {
                write_letter_trial(letter, PLID);
                do_rlg("NOK", index);
            }
        }
        else {
            if (strcmp(active_games[index]->curr_word_guess, active_games[index]->word)==0){
                write_letter_trial(letter, PLID);
                end_game_file_create(active_games[index]->player_plid, 'W');
                do_rlg("WIN", index);
                delete_active_game(PLID);
            }
            else {
                do_rlg_ok_msg(index, letter);
                write_letter_trial(letter, PLID);
            }
        }
    }
    return 0;
}

int do_pwg(char* PLID, char* word, int trials) {
    int index;
    char buffer[128];
    index = find_player_game_index(PLID);
    if (index==-1){
        sprintf(buffer, "RWG ERR\n");
        n=sendto(fd, buffer, 8, 0, (struct sockaddr*)&addr,addrlen);
        if(n==-1) exit(1); 
    }
    else if (active_games[index]->trials + 1 != trials) {
        sprintf(buffer, "RWG INV %d\n", active_games[index]->trials);
        n=sendto(fd, buffer, 128, 0, (struct sockaddr*)&addr,addrlen);
        if(n==-1) exit(1); 
    }
    else {
        write_word_guess(word, PLID);
        active_games[index]->trials += 1;
        if (strcmp(word,active_games[index]->word)==0){
            end_game_file_create(active_games[index]->player_plid, 'W');
            sprintf(buffer, "RWG WIN %d\n", active_games[index]->trials);
            n=sendto(fd, buffer, 128, 0, (struct sockaddr*)&addr,addrlen);
            if(n==-1) exit(1);
            delete_active_game(PLID);
        }
        else{
            active_games[index]->errors++;
            if (active_games[index]->max_errors <= active_games[index]->errors){
                end_game_file_create(active_games[index]->player_plid, 'F');
                sprintf(buffer, "RWG OVR %d\n", active_games[index]->trials);
                n=sendto(fd, buffer, 128, 0, (struct sockaddr*)&addr,addrlen);
                if(n==-1) exit(1); 
                delete_active_game(PLID);
            }
            else {
                sprintf(buffer, "RWG NOK %d\n", active_games[index]->trials);
                n=sendto(fd, buffer, 128, 0, (struct sockaddr*)&addr,addrlen);
                if(n==-1) exit(1); 
            }
        }
    }
    return 0;
}

int do_qut(char* PLID){
    int index;
    char buffer[128];
    index = find_player_game_index(PLID);
    if (index == -1){
        n=sendto(fd, "RQT ERR\n", 8, 0, (struct sockaddr*)&addr,addrlen);
        if(n==-1) exit(1); 
    }
    else{
        end_game_file_create(active_games[index]->player_plid, 'Q');
        n=sendto(fd, "RQT OK\n", 7, 0, (struct sockaddr*)&addr,addrlen);
        if(n==-1) exit(1); 
    }
    return 0;
}

int do_rev(char *PLID){
    int index;
    char buffer[128];
    index = find_player_game_index(PLID);
    if (index==-1){
        sprintf(buffer, "RRV ERR\n");
        n=sendto(fd, buffer, 8, 0, (struct sockaddr*)&addr,addrlen);
        if(n==-1) exit(1); 
    }
    else {
        sprintf(buffer, "RRV %s/OK\n", active_games[index]->word);
        n=sendto(fd, buffer, 9+strlen(active_games[index]->word), 0, (struct sockaddr*)&addr,addrlen);
        if(n==-1) exit(1); 
    }
    return 0;
}

int create_active_game(int index, char plid[7]){
    active_games[index] = (Game *) malloc(sizeof(Game));
    active_games[index]->word = (char*) malloc(MAX_WORD_SIZE);
    strcpy(active_games[index]->player_plid, plid);
    active_games[index]->word = get_random_word(word_filename);
    active_games[index]->max_errors = get_max_errors(active_games[index]->word);
    active_games[index]->trials = 0;
    active_games[index]->errors = 0;
    active_games[index]->curr_word_guess = (char*) malloc(64*sizeof(char));
    for (int c = 0; c < strlen(active_games[index]->word); c++){
        active_games[index]->curr_word_guess[c] = '_';
    }
    active_games[index]->word[strlen(active_games[index]->word)] = '\0';
    active_games[index]->curr_word_guess[strlen(active_games[index]->word)] = '\0';
    return 0;
}

int do_rsg(char status [], int index){
    if (strcmp(status, "NOK")==0){
        strcpy(buffer, "RSG NOK\n");
        n=sendto(fd, buffer, 8, 0, (struct sockaddr*)&addr,addrlen);
        if(n==-1) 
            exit(1);
    }
    else if (strcmp(status, "OK")==0){
        sprintf(buffer, "RSG OK %ld %d\n", strlen(active_games[index]->word), active_games[index]->max_errors);
        n=sendto(fd, buffer, 12, 0, (struct sockaddr*)&addr,addrlen);
        if(n==-1) 
            exit(1);
    }
    else{
        return 1;
    }
    return 0;
}

int do_rlg(char status [4], int index){
    if (strcmp(status, "ERR")==0){
        sprintf(buffer, "RLG ERR\n");
    }
    else if (strcmp(status, "INV")==0){
        sprintf(buffer, "RLG INV %d\n", active_games[index]->trials);
    }
    else if (strcmp(status, "DUP")==0){
        sprintf(buffer, "RLG DUP %d\n", active_games[index]->trials);
    }
    else if (strcmp(status, "OVR")==0){
        sprintf(buffer, "RLG OVR %d\n", active_games[index]->trials);
    }
    else if (strcmp(status, "NOK")==0){
        sprintf(buffer, "RLG NOK %d\n", active_games[index]->trials);
    }
    else if (strcmp(status, "WIN")==0){
        sprintf(buffer, "RLG WIN %d\n",active_games[index]->trials);
    }
    n=sendto(fd, buffer, 128, 0, (struct sockaddr*)&addr,addrlen);
        if(n==-1) exit(1);
    return 0;
}

int receive_cmd_arguments(int argc, char *argv []){
    if (argc < 2){
        printf("Error: You need to include a word_file file.\n");
        return 1;
    }
    strcpy(word_filename, argv[1]);
    if (argc==3){
        strcpy(port, argv[2]);
    }
    else {
        strcpy(port, DEFAULT_PORT);
    }
    return 0;
}

int make_udp_connection(){
    fd=socket(AF_INET,SOCK_DGRAM,0);
    if(fd==-1) exit(1);
    memset(&hints,0,sizeof hints);
    hints.ai_family=AF_INET; 
    hints.ai_socktype=SOCK_DGRAM;
    hints.ai_flags=AI_PASSIVE;
    errcode = getaddrinfo(NULL, port, &hints,&res);
    if(errcode!=0) exit(1);
    n=bind(fd,res->ai_addr, res->ai_addrlen);
    if(n==-1) exit(1);
    return 0;
}