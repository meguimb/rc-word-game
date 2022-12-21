#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <stdio.h>
#include "clientTCP.h"
#define PORT "58011"

#ifndef CLIENT_TCP_H
#define CLIENT_TCP_H
int fd, errcode;
ssize_t n;
socklen_t addrlen;
struct addrinfo hints, *res;
struct sockaddr_in addr;
char buffer[128];
#endif

void connect_to_server() {
    fd=socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1) {exit(1);}
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    errcode = getaddrinfo("tejo.tecnico.ulisboa.pt", PORT, &hints, &res);
}

void scoreboard() {
    char status[5], info[28], Fname[24], Fsize[3];
    int DataSize;
    char *Fdata;
    FILE *fp;
    connect_to_server();
    n=connect(fd, res->ai_addr, res->ai_addrlen);
    if(n == -1) {exit(1);}
    n=write(fd, "GSB\n", 4);
    if(n == -1) {exit(1);}
    n=read(fd, buffer, 128);
    if(n == -1) {exit(1);}
    sscanf(buffer, "RSB %s", status);
    if(strcmp(status, "OK") == 0) {
        n=read(fd, info, 28);
        if(n == -1) {exit(1);}
        sscanf(info, "%s %s", Fname, Fsize);
        DataSize = atoi(Fsize);
        Fdata = malloc(DataSize);
        n=read(fd, Fdata, DataSize);
        if(n == -1) {exit(1);}
        fp = fopen(Fname, "w");
        if(fp == NULL) {exit(1);}
        fprintf(fp, "%s", Fdata);
        printf("%s", Fdata);
        fclose(fp);
    }
    else if(strcmp(status, "EMPTY") == 0) {
        printf("No games played yet.\n");
    }
    else {
        printf("Error");
    }
    close(fd);
}

void hint(char* PLID) {
    char status[3], info[41], Fname[30], Fsize[10], block[256];
    int DataSize;
    char *Fdata;
    char *msg = malloc(11);
    FILE *fp;
    connect_to_server();
    n=connect(fd, res->ai_addr, res->ai_addrlen);
    if(n == -1) {exit(1);}
    sprintf(msg, "GHL %s\n", PLID);
    n=write(fd, msg, 11);
    if(n == -1) {exit(1);}
    n=read(fd, buffer, 128);
    if(n == -1) {exit(1);}
    sscanf(buffer, "RHL %s", status);
    if(strcmp(status, "OK") == 0) {
        n=read(fd, info, 41);
        if(n == -1) {exit(1);}
        sscanf(info, "%s %s", Fname, Fsize);
        DataSize = atoi(Fsize);
        Fdata = malloc(DataSize);
        for(int i = 0; i < DataSize; i=i+256) {
            if(DataSize - i < 256) {n=read(fd, block, DataSize - i);}
            else {n=read(fd, block, 256);}
            if(n == -1) {exit(1);}
            strcat(Fdata, block);
            if(n == 0) {break;}
        }
        fp = fopen(Fname, "w");
        if(fp == NULL) {exit(1);}
        fprintf(fp, "%s", Fdata);
        fclose(fp);
    }
    else if(strcmp(status, "EMPTY") == 0) {printf("No games played yet.\n");}
    else {printf("Error\n");}
    close(fd);
}

void state(char* PLID) {
    char status[3], info[21], Fname[24], Fsize[3];
    int DataSize;
    char *Fdata;
    char *msg = malloc(11);
    FILE *fp;
    connect_to_server();
    n=connect(fd, res->ai_addr, res->ai_addrlen);
    if(n == -1) {exit(1);}
    sprintf(msg, "STA %s\n", PLID);
    n=write(fd, msg, 11);
    if(n == -1) {exit(1);}
    n=read(fd, buffer, 128);
    if(n == -1) {exit(1);}
    sscanf(buffer, "RST %s", status);
    if(strcmp(status, "ACT") == 0 || strcmp(status, "FIN") == 0) {
        n=read(fd, info, 21);
        if(n == -1) {exit(1);}
        sscanf(info, "%s %s", Fname, Fsize);
        DataSize = atoi(Fsize);
        Fdata = malloc(DataSize);
        n=read(fd, Fdata, DataSize);
        if(n == -1) {exit(1);}
        fp = fopen(Fname, "w");
        if(fp == NULL) {exit(1);}
        fprintf(fp, "%s", Fdata);
        printf("%s", Fdata);
        fclose(fp);
    }
    else {
        printf("No active or finished games.\n");
    }
    close(fd);
}