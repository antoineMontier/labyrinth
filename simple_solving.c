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

#define CHEMIN_LENGTH 100

typedef int** laby;

typedef struct{
    int col;
    int line;
} slot;

typedef slot* chemin;

laby create_labyrinth(int cols, int lines);
int is_solution_true(laby, slot*, int);
void free_labyrinth(laby*, int cols, int lines);
void print_labyrinth(laby , int cols, int lines);
slot* solve_labyrinth(laby, int cols, int lines);
void print_slot(slot);
int rec_find(laby, chemin res, slot start, slot end);
int slot_in_chemin(int col, int line, chemin c);
void add_current_coordinates_to_chemin(int col, int line, chemin c);

int main(){
    laby l = create_labyrinth(COLS, LINES);
    print_labyrinth(l, COLS, LINES);
    solve_labyrinth(l, COLS, LINES);
    free_labyrinth(&l, COLS, LINES);
    return 0;
}

void add_current_coordinates_to_chemin(int col, int line, chemin c){
    for(int i = 0 ; i < CHEMIN_LENGTH; i++)
        if(c[i].col == -1 && c[i].line == -1){
            slot s = {col, line};
            c[i] = s;
            return;
        }
}


int slot_in_chemin(int col, int line, chemin c){
    for(int i=0; i<CHEMIN_LENGTH; i++)
        if(c[i].col == col && c[i].line == line)
            return 1;
    return 0;
}

int rec_find(laby l, chemin res, slot current, slot end){
    if(res == NULL)
        fprintf(stderr, "chemin is null, use malloc\n");
    add_current_coordinates_to_chemin(current.col, current.line, res);
    if(current.col == end.col && current.line == end.line){
        printf("solved\n");
        print_slot(current);
        return 1;
    }
    print_slot(current);
    // check 4 directions

    if(!slot_in_chemin(current.col-1, current.line, res) && l[current.col-1][current.line] != WALL){
        slot next = {current.col-1, current.line};
        rec_find(l, res, next, end);   
    }
    if(!slot_in_chemin(current.col+1, current.line, res) && l[current.col+1][current.line] != WALL){
        slot next = {current.col+1, current.line};
        rec_find(l, res, next, end);
    }
    if(!slot_in_chemin(current.col, current.line-1, res) && l[current.col][current.line-1] != WALL){
        slot next = {current.col, current.line-1};
        rec_find(l, res, next, end);
    }
    if(!slot_in_chemin(current.col, current.line+1, res) && l[current.col][current.line+1] != WALL){
        slot next = {current.col, current.line+1};
        rec_find(l, res, next, end);
    }   
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

    //let's find the end slot
    slot end;
    for(int i=0; i< cols; i++)
        for(int j=0; j< lines; j++)
            if(l[i][j] == EXIT){
                end.col = i;
                end.line = j;
                //stop the loops
                i = cols;
                j = lines;
            }

    chemin answer = malloc(sizeof(chemin) * CHEMIN_LENGTH);
    for(int i = 0; i < CHEMIN_LENGTH; i++){
        answer[i].col = -1;
        answer[i].line = -1;
    }
    rec_find(l, answer, start, end);

    free(answer);
}

void print_slot(slot s){
    printf("%d %d\n", s.col, s.line);
}




void print_labyrinth(laby l, int cols, int lines){
    for(int i=0; i< cols; i++){
        for(int j=0; j< lines; j++)
            switch(l[i][j]){
                case WALL:
                    printf("#");
                    break;
                case PATH:
                    printf(" ");
                    break;
                case ENTRY:
                    printf("D");
                    break;
                case EXIT:
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
        else if(slot_array[i].line == slot_array[i-1].line){
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