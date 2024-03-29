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
#include <dirent.h>

#include "server.c"

#define PORT "58011"
#define MAX_GAMES 10

typedef struct scores {
    int *score;
    char *player_plid[7];
    char *word;
    int *n_succ;
    int *n_total;
} Scores;

int receive_gsb_command(void);
int receive_ghl_command(char plid[7]);
int receive_sta_command(char plid[7]);
int find_last_game(char plid[7], char *fname);
int find_top_scores(Scores *list);
int does_player_have_active_games(char plid[7]);
int find_player_game_index(char *PLID);

int fd, errcode;
ssize_t n;
socklen_t addrlen;
struct addrinfo hints, *res;
struct sockaddr_in addr;
char buffer[128];
Scores *scorelist[MAX_GAMES];

int mn(void){
    char PLID[7];
    int newfd;

    fd=socket(AF_INET,SOCK_STREAM,0); 
    if (fd==-1) 
        exit(1); 
    memset(&hints,0,sizeof hints);
    hints.ai_family=AF_INET; 
    hints.ai_socktype=SOCK_STREAM;
    hints.ai_flags=AI_PASSIVE;
    errcode=getaddrinfo(NULL,PORT,&hints,&res);
    if((errcode)!=0)
        exit(1);
    n=bind(fd,res->ai_addr,res->ai_addrlen);
    if(n==-1) 
        exit(1);
    if(listen(fd,5)==-1)
        exit(1);
    addrlen=sizeof(addr);
    if((newfd=accept(fd,(struct sockaddr*)&addr, &addrlen))==-1)
            exit(1);
    n=read(newfd, buffer, 128);
    if(n==-1)
        exit(1);
    write(1,"received: ",10);
    write(1,buffer,n);
    n=write(newfd,buffer,n);
    if(n==-1)
        exit(1);
    // check command coming from player.c   
    if (sscanf(buffer, "GSB\n"))
        receive_gsb_command();

    else if(sscanf(buffer, "GHL %s\n", PLID))
        receive_ghl_command(PLID); 

    else if(sscanf(buffer, "STA %s\n", PLID))
        receive_sta_command(PLID);

    close(newfd);
    
    freeaddrinfo(res);
    close(fd);
    return 0;
}  

int receive_gsb_command(void){
    char response_buffer[64];
    int i, sum;
    int scoreboard[10];
    if (find_top_scores(scorelist) == 0){
        // GSB NOK
        strcpy(response_buffer, "status = EMPTY\n");
        n=sendto(fd, response_buffer, 16, 0, (struct sockaddr*)&addr,addrlen);
        if(n==-1)
            exit(1);
        return 1;
    }
    printf("status = OK\n");
    for (i = 0; i < 10; i++){
        printf("%d %s %s %d %d\n", scorelist[i]->score, scorelist[i]->player_plid, scorelist[i]->word, scorelist[i]->n_succ, scorelist[i]->n_total);
        sum = strlen(scorelist[i]->score) + strlen(scorelist[i]->player_plid) + strlen(scorelist[i]->word) + strlen(scorelist[i]->n_succ) + strlen(scorelist[i]->n_total) + 6;
        n=sendto(fd, response_buffer, sum, 0, (struct sockaddr*)&addr,addrlen);
        if(n==-1)
            exit(1);
    }
    return 0;

}        


int receive_ghl_command(char plid[7]){
    char response_buffer[128];
    char player_filename[16], image_filename[16];
    char word[16];
    char ch;
    FILE *fp, *image;
    
    printf("status = OK\n");
    if (find_last_game(plid, response_buffer) == 0){
        // STA NOK
        strcpy(response_buffer, "status = NOK\n");
        n=sendto(fd, response_buffer, 14, 0, (struct sockaddr*)&addr,addrlen);
        if(n==-1)/*error*/
            exit(1);
        return 1;
    }
    sprintf(player_filename, "GAMES/GAME_%s.txt", plid);
    fp = fopen(player_filename, "r");
    ch = fgetc(fp);
    while (ch != " "){
        strncat(word, &ch, 1);
        ch = fgetc(fp);
    }
    fclose(fp);
    printf("word = %s\n", word);

    if (find_image_file(word) == 0){
        strcpy(response_buffer, "status = NOK\n");
        n=sendto(fd, response_buffer, 14, 0, (struct sockaddr*)&addr,addrlen);
        if(n==-1)/*error*/
            exit(1);
        return 1;
    }
    sprintf(image_filename, "images/%s.jpg", word);
    image = fopen(image_filename, "r");
    //display da imagem no ficheiro
    n=sendto(fd, image, 14, 0, (struct sockaddr*)&addr,addrlen);
    if(n==-1)/*error*/
        exit(1);
    fopen(image_filename, "r");
    fclose(image);
}

int receive_sta_command(char plid[7]){
    char response_buffer[128];
    int i, sum;
    char player_filename[16];
    Scores *list;

    if (find_last_game(plid, response_buffer) == 0){
        // STA NOK
        strcpy(response_buffer, "status = NOK\n");
        n=sendto(fd, response_buffer, 14, 0, (struct sockaddr*)&addr,addrlen);
        if(n==-1)/*error*/
            exit(1);
        return 1;
    }
    else if (does_player_have_active_games(plid) == 0){
        strcpy(response_buffer, "status = ACT\n");
        n=sendto(fd, response_buffer, 14, 0, (struct sockaddr*)&addr,addrlen);
        if(n==-1)/*error*/
            exit(1);
        return 1;
        i = find_player_game_index(plid);
        printf("%s %s %d %d %d\n", active_games[i]->player_plid, active_games[i]->word, active_games[i]->curr_word_guess, active_games[i]->errors, active_games[i]->trials);
        sum = strlen(active_games[i]->player_plid) + strlen(active_games[i]->word) + strlen(active_games[i]->curr_word_guess) + strlen(active_games[i]->errors) + strlen(active_games[i]->trials) + 6;
        n=sendto(fd, response_buffer, sum, 0, (struct sockaddr*)&addr,addrlen);
        if(n==-1)/*error*/
            exit(1);
    }
    /*
    else if (find_last_game(plid, response_buffer) == 1){
        strcpy(response_buffer, "status = FIN\n");
        n=sendto(fd, response_buffer, 14, 0, (struct sockaddr*)&addr,addrlen);
        if(n==-1)
            exit(1);
        return 1;
        i = find_player_game_index(plid);
        printf("%s %s %d %d %d %d\n", active_games[i]->player_plid, active_games[i]->word, active_games[i]->curr_word_guess, active_games[i]->errors, active_games[i]->trials, active_games[i]->score);
        sum = strlen(active_games[i]->player_plid) + strlen(active_games[i]->word) + strlen(active_games[i]->curr_word_guess) + strlen(active_games[i]->errors) + strlen(active_games[i]->trials) + strlen(active_games[i]->score) + 7;
        n=sendto(fd, response_buffer, sum, 0, (struct sockaddr*)&addr,addrlen);
        if(n==-1)
            exit(1);
    }*/
    
}

int find_top_scores(Scores *list) {
    struct dirent **filelist;
    int n_entries, i_file;
    char fname[50];
    FILE *fp;

    n_entries = scandir("SCORES/", &filelist, 0, alphasort);

    i_file = 0;
    if (n_entries <0)
        return 0;

    else{
        while (n_entries--){
            if(filelist[n_entries]->d_name[0]!='.'){
                sprintf(fname, "SCORES/%s", filelist[n_entries]->d_name);
                fp = fopen(fname, "r");
                if (fp == NULL){
                    fscanf(fp, "%d %s %s %d %d", &list->score[i_file], list->player_plid[i_file], list->word[i_file], &list->n_succ[i_file], &list->n_total[i_file]);
                    fclose(fp);
                    ++i_file;
                }   
            }

            free(filelist[n_entries]);
            if(i_file==10)
                break;
        }
        free(filelist);
    } 

    list->score=i_file;
    return(i_file);   

} 

int find_last_game(char plid[7], char *fname){
    struct dirent **filelist;
    int n_entries, found;
    char dirname[20];

    sprintf(dirname, "GAMES/%s", plid);
    n_entries = scandir(dirname, &filelist, 0, alphasort);
    found = 0;

    if (n_entries <= 0)
        return 0;
    else{
        while (n_entries--){
            if (filelist[n_entries]->d_name[0]!='.'){
                sprintf(fname, "GAMES/%s/%s", plid, filelist[n_entries]->d_name);
                found = 1;
            }
            free(filelist[n_entries]);
            if(found)
                break;
        }
        free(filelist);
    } 
    return(found);   
}

int does_player_have_active_games(char plid[7]){
    char player_filename[16];
    FILE *file;
    sprintf(player_filename, "GAMES/GAME_%s.txt", plid);
    if ((file = fopen(player_filename, "r")))
    {
        fclose(file);
        return 1;
    }
    printf("no file for this user\n");
    return 0;
}

int find_image_file(char word[16]){
    char image_filename[16];
    FILE *file;
    sprintf(image_filename, "images/%s.jpg", word);
    if ((file = fopen(image_filename, "r")))
    {
        fclose(file);
        return 1;
    }
    printf("no file image\n");
    return 0;
}
