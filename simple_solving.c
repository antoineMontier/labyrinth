// 0 = wall, 1 = path, 2 = entry , 3 = exit

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


#define WALL 0
#define PATH 1
#define ENTRY 2
#define EXIT 3

typedef int** laby;

typedef struct{
    int col;
    int line;
} slot;

laby create_labyrinth(int cols, int lines){
    char* command = malloc(128);
    sprintf(command, "python generateur.py %d %d > out.txt", cols, lines);
    system(command);
    free(command);
}

int is_solution_true(laby, slot*, int);

int main(){
    create_labyrinth(11, 11);
    return 0;
}

int is_solution_true(laby l, slot*slot_array, int path_length){
    //check if beginning is entry and end is exit

    if(l[slot_array[0].col][slot_array[0].line] != ENTRY)
        return 0; //false
    if(l[slot_array[path_length - 1].col][slot_array[path_length - 1].line] != EXIT)
        return 0; //false
    //check if each slots are neightbours
    for(int i = 1; i < path_length ; i++){
        //same col
        if(slot_array[i].col == slot_array[i-1].col){
            //check the line difference between the two lines is 1.
            if(fabs(slot_array[i].line - slot_array[i-1].line) != 1)
                return 0; // not neighbours
        }
        //same line
        else if(slot_array[i].line == slot_array[i].line){
            //check the col difference between the two lines is 1
            if(fabs(slot_array[i].col - slot_array[i-1].col) != 1)
                return 0; // not neighbours
        }
        else{
            return 0;//not neightbours
        }
    }
    return 1; // all tests passed
}
