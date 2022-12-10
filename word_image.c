#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <word_image.h>


char *get_random_line(char filename [64]){
    char *word = (char*) malloc(64*sizeof(char));

    FILE *fp;
    int i=0, totalWords, random_index;

    totalWords = (int) get_number_of_lines(filename);
    random_index = get_random_index(totalWords);
    fp = fopen(filename, "r");
    while (i != random_index){
        fgets(word, 64, fp);
        i++;
    }
    return word;
}

int get_number_of_lines(char filename [64]){
    FILE *fp;
    int count_lines = 0;
    char chr;

    fp = fopen(filename, "r");
 
    chr = getc(fp);
    while (chr != EOF){
        if (chr == 'n'){
            count_lines = count_lines + 1;
        }
        chr = getc(fp);
    }
    fclose(fp); 
    return count_lines-1;
}

int get_random_index(int max){
    srand(time(NULL));   // Initialization, should only be called once.
    int r = rand() % max; 
    return r;
}

char *get_word_from_line(char *line){
    char *word = (char*) malloc(64*sizeof(char)), *words;
    char delim[] = " ";
    words = strtok(line, delim);
    strcpy(word, words);
    words = strtok(NULL, delim);
    return word;
}

