// 0 = wall, 1 = path, 2 = entry , 3 = exit

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


#define WALL 0
#define PATH 1
#define ENTRY 2
#define EXIT 3

typedef int** laby;

laby create_labyrinth(int cols, int lines){
    char* command = malloc(128);
    sprintf(command, "python generateur.py %d %d > out.txt", cols, lines);
    system(command);
    free(command);
    FILE*
}

int main(){
    create_labyrinth(11, 11);
    return 0;
}