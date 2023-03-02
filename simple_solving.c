// 0 = wall, 1 = path, 2 = entry , 3 = exit

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>


#define WALL 0
#define PATH 1
#define ENTRY 2
#define EXIT 3

#define COLS 11
#define LINES 11

typedef int** laby;

typedef struct{
    int col;
    int line;
} slot;

laby create_labyrinth(int cols, int lines);
int is_solution_true(laby, slot*, int);
void free_labyrinth(laby*, int cols, int lines);
void print_labyrinth(laby , int cols, int lines);
slot* solve_labyrinth(laby, int cols, int lines);
void print_slot(slot);

int main(){
    laby l = create_labyrinth(COLS, LINES);
    print_labyrinth(l, COLS, LINES);
    solve_labyrinth(l, COLS, LINES);
    free_labyrinth(&l, COLS, LINES);
    return 0;
}

slot* solve_labyrinth(laby l, int cols, int lines){
    // first let's find the depart
    slot start;
    for(int i=0; i< cols; i++)
        for(int j=0; j< lines; j++)
            if(l[i][j] == ENTRY){
                start.col = i;
                start.line = j;
                //stop the loops
                i = cols;
                j = lines;
            }


}

void print_slot(slot s){
    printf("%d %d\n", s.col, s.line);
}




void print_labyrinth(laby l, int cols, int lines){
    for(int i=0; i< cols; i++){
        for(int j=0; j< lines; j++)
            switch(l[i][j]){
                case 0:
                    printf("#");
                    break;
                case 1:
                    printf(" ");
                    break;
                case 2:
                    printf("D");
                    break;
                case 3:
                    printf("X");
                    break;
                default:
                    break;
            }
        printf("\n");
    }
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

void free_labyrinth(laby*l, int cols, int lines){
    if(*l == NULL)
        return; // nothing to do

    for(int i = 0 ; i < cols ; i++)
        if((*l)[i] != NULL)
            free((*l)[i]);
    if(*l != NULL)
        free(*l);
    *l = NULL;
}

laby create_labyrinth(int cols, int lines){
    char* command = malloc(128);
    sprintf(command, "python3 generateur.py %d %d > out.txt", cols, lines);
    system(command);
    free(command);
    FILE* out = fopen("out.txt", "r");
    if (out == NULL) {
        printf("Failed to open file.\n");
    }

    laby current_laby = (laby)malloc(cols * sizeof(int *));
    for (int i = 0; i < lines; i++) {
        current_laby[i] = (int *)malloc(lines * sizeof(int));
    }

    int c;

    for (int i = 0; i < cols; i++) {
        for (int j = 0; j < lines; j++) {
            c = fgetc(out);
            if (c != '\n') {
                current_laby[i][j] = c - '0';
            }
        }
        fgetc(out); // skip over the newline character
    }
    
    fclose(out);
    return current_laby;
}