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
#define PORT "58011"

// int receive_gsb_command(void);
int receive_ghl_command(char plid[7]);
// int receive_sta_command(char plid[7]);

int fd, errcode;
ssize_t n;
socklen_t addrlen;
struct addrinfo hints, *res;
struct sockaddr_in addr;
char buffer[128];

void main(void){
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
    if(newfd=accept(fd,(struct sockaddr*)&addr, &addrlen))==-1
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
    /*if (sscanf(buffer, "GSB\n"))
        receive_gsb_command();*/

    /*else if(sscanf(buffer, "GHL %s\n", PLID))
        receive_gwl_command(PLID);*/ 

    /*else*/  if(sscanf(buffer, "STA %s\n", PLID))
        receive_sta_command(PLID);

    close(newfd);
    
    freeaddrinfo(res);
    close(fd);
}    

int receive_sta_command(char plid[7]){
    char response_buffer[128];
    if (find_last_game(plid, response_buffer) == 0){
        // STA NOK
        strcpy(response_buffer, "status = NOK\n");
        n=sendto(fd, response_buffer, 14, 0, (struct sockaddr*)&addr,addrlen);
        if(n==-1)/*error*/
            exit(1);
        return 1;
    }
    if (does_player_have_active_game(plid) == 0){
        //criar ficheiro que tenha as guesses da pessoa, palavra certa e hint e e isso que e impresso nesta função, se tiver terminado manda o resultado e tal
        strcpy(response_buffer, "status = FIN\n");
        n=sendto(fd, response_buffer, 14, 0, (struct sockaddr*)&addr,addrlen);
        if(n==-1)/*error*/
            exit(1);
        return 1;
    }

    if (does_player_have_active_game(plid) == 1){
        strcpy(response_buffer, "status = ACT\n");
        n=sendto(fd, response_buffer, 14, 0, (struct sockaddr*)&addr,addrlen);
        if(n==-1)/*error*/
            exit(1);
        return 1;
    }

}

int find_last_game(int plid, char *fname){
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

/*
int receive_gsb_command(void){
    char response_buffer[64];
    if (is_scoreboard_empty(scorelist->scores) == 0){
        // GSB NOK
        strcpy(response_buffer, "status = EMPTY\n");
        n=sendto(fd, response_buffer, 16, 0, (struct sockaddr*)&addr,addrlen);
        if(n==-1)
            exit(1);
        return 1;
    }
    if(n==-1)
        exit(1);
    return 0;
    printf("status = OK\n");

}

int is_scoreboard_empty(scores *list) {
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
                    fscanf(fp, "%d %s %s %d %d", &list->score[i_file], list->PLID[i_file], list->word[i_file], &list->n_succ[i_file], &list->n_tot[i_file]);
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

    list->n_scores=i_file;
    return(i_file);   

} */

int does_player_have_active_game(char plid [7]){
    char player_filename [16];
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
