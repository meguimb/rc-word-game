#include <stdio.h>
#include <string.h>

int do_start_command(char plid [7]);
int do_play_command(char letter);
int do_guess_command(char word [64]);
int do_scoreboard_command();
int do_hint_command();
int do_state_command();
int do_quit_command();
int do_exit_command();

int main(){
    char letter, input[16], word[64], PLID[7];

    fgets(input, sizeof input, stdin);
    if (sscanf(input, "start %6s", PLID) || sscanf(input, "sg %6s", PLID)){
        do_start_command(PLID);
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

    return 0;
}

int do_start_command(char plid [7]){
    printf("your student id number is: %s\n", plid);
    // sends message to Game Server using UDP to start a new game
    return 0;
}

int do_play_command(char letter){
    printf("you chose to play the letter %c\n", letter);
    return 0;
}

int do_guess_command(char word [64]){
    printf("you tried to guess the word %s\n", word);
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
    printf("quitting the game...\n");
    return 0;
}

int do_exit_command(){
    printf("exiting the game...\n");
    return 0;
}