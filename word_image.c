#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <word_image.h>

int word_index = 0;

char *get_line(char filename [64], int max_words){
    FILE *fp;
    int i=0, totalWords;
    char *word = (char*) malloc(64*sizeof(char));

    fp = fopen(filename, "r");
    fgets(word, 64, fp);
    while (i != word_index){
        fgets(word, 64, fp);
        i++;
    }
    word_index = (word_index + 1) % max_words;
    return word;
}

int get_number_of_lines(char filename [64]){
    FILE *fp;
    int count_lines = 0;
    char chr;

    fp = fopen(filename, "r");
 
    chr = getc(fp);
    while (chr != EOF){
        if (chr == '\n'){
            count_lines = count_lines + 1;
        }
        chr = getc(fp);
    }
    fclose(fp); 
    return count_lines;
}


char *get_word_from_line(char *line){
    char *word = (char*) malloc(64*sizeof(char)), *words;
    char delim[] = " ";
    words = strtok(line, delim);
    strcpy(word, words);
    words = strtok(NULL, delim);
    return word;
}

