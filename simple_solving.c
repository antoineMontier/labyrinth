// 0 = MUR, 1 = WAY, 2 = ENTREE , 3 = exit

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>


#define MUR 0
#define WAY 1
#define ENTREE 2
#define EXIT 3
#define VISITE (-124)
#define END_SIGNAL (-123)

#define COLS 9
#define LINES 7

#define CHEMIN_LENGTH (COLS * LINES / 2)

typedef int** laby;

typedef struct{
    int col;
    int line;
} Case;

typedef Case* chemin;

laby creer_labyrinth(int cols, int lines);
int check_solution(laby, chemin);
void free_labyrinth(laby*, int cols, int lines);
void print_labyrinth(laby , int cols, int lines);
Case* solve_labyrinth(laby, int cols, int lines);
void print_Case(Case);
void rec_find(laby, chemin res, Case start, Case end);
int Case_in_chemin(int col, int line, chemin c);
void ajouter_coordonees_au_chemin_au_dernier_voisin(int col, int line, chemin c);
void print_chemin(chemin c);
void print_solution(laby l, int cols, int lines, chemin c);
int abs(int x);
int sont_voisines(Case a, Case b); // ne fonctionne pas en diagonale
int cases_egales(Case a, Case b);
int ajouter_au_dernier_voisin(chemin c, Case a_ajouter);
void nettoyer_chemin(chemin c); // le probleme est que parfois les cases se suivent bien sauf entre l'avant derniere et la dernier (=arrivee) il s'intercale des cases deja visitees

int main(){
    laby l = creer_labyrinth(COLS, LINES);
    print_labyrinth(l, COLS, LINES);
    chemin ans = solve_labyrinth(l, COLS, LINES);
    print_chemin(ans);
    if(check_solution(l, ans))
        printf("Solution OK\n");
    
    print_solution(l, COLS, LINES, ans);
    free(ans);
    free_labyrinth(&l, COLS, LINES);
    return 0;
}

void nettoyer_chemin(chemin c){
    // se placer à l'indice fin : 
    int ind_fin = CHEMIN_LENGTH-2;
    while(ind_fin >= 0 && c[ind_fin].col == -1 && c[ind_fin].line == -1) ind_fin--;

    if(ind_fin == 0) // chemin n'a aucune case valable, rien à nettoyer
        return;
    
    // chercher le premier voisin en continuant de remonter vers le depart
    while(ind_fin >=0 && !sont_voisines(c[ind_fin], c[ind_fin-1])){
        c[ind_fin -1] = c[ind_fin];
        c[ind_fin] = (Case){-1 , -1};
        --ind_fin;
    }
}


void print_solution(laby l, int cols, int lines, chemin c){
    // ajouter des lettres dans le labyrinth pour montrer le chemin pris
    for(int i=1; i<CHEMIN_LENGTH-1 ; ++i){
        if(c[i+1].col == -1 && c[i+1].line == -1)
            break;
        else
            l[c[i].col][c[i].line] = i < 27 ? 'a' + i - 1 : 'A' + i - 27; 
        }

    print_labyrinth(l, cols, lines);

    // retirer les lettres
    for(int i=1; i<CHEMIN_LENGTH ; ++i){
        if(c[i].col == -1 && c[i].line == -1)
            break;
        else
            l[c[i].col][c[i].line] = WAY;
        }
}


void print_chemin(chemin c){
    printf("[");
    for(int i = 0; i < CHEMIN_LENGTH ; i++){
        if(c[i].col == -1 && c[i].line == -1)
            break;
        printf(" %d;%d ", c[i].col, c[i].line);
    }
    printf("]\n");
}


void ajouter_coordonees_au_chemin_au_dernier_voisin(int col, int line, chemin c){
    if(c[CHEMIN_LENGTH-1].col == END_SIGNAL && c[CHEMIN_LENGTH-1].line == END_SIGNAL)
        return; // end signal : solution deja trouvee
    Case s = {col, line};
    // printf("about to add %d;%d\n", col, line  );
    for(int i = 0 ; i < CHEMIN_LENGTH; i++)
        if(c[i].col == -1 && c[i].line == -1){
            if(i == 0){
                c[i] = s;
                return;
            }else{
                ajouter_au_dernier_voisin(c, s);
            }
    }
}

int ajouter_au_dernier_voisin(chemin c, Case a_ajouter){
    for(int i = CHEMIN_LENGTH-2; i >= 0; i--){
        if(!(c[i].col == -1 && c[i].line == -1) && sont_voisines(c[i], a_ajouter)){// skipper toutes les cases à la fin de coordonnees{-1 ; 1}
            c[i+1] = a_ajouter;
            return 1; // case ajoutee
        }
    }
    return 0; // aucune case ajoutee
}


int Case_in_chemin(int col, int line, chemin c){
    for(int i=0; i<CHEMIN_LENGTH; i++)
        if(c[i].col == col && c[i].line == line)
            return 1;
    return 0;
}

void rec_find(laby l, chemin res, Case current, Case end){

    if(res == NULL)
        fprintf(stderr, "chemin doit etre declare avec malloc\n");

    if(res[CHEMIN_LENGTH-1].col == END_SIGNAL && res[CHEMIN_LENGTH-1].line == END_SIGNAL) // check end_signal
        return; // une solution a deja ete trouvee
    
    if(cases_egales(current, end)){

        // ajouter à la main la derniere case dans le chemin :
        for(int i=0; i<CHEMIN_LENGTH; i++)
            if(res[i].col == -1 && res[i].line == -1){
                res[i] = end;
                break;
            }


        Case end_signal = {END_SIGNAL, END_SIGNAL};
        res[CHEMIN_LENGTH-1] = end_signal;
        return;
    }

    // marquer la case comme visitée : 
    l[current.col][current.line] = VISITE;

    //ajouter la case dans le chemin
    ajouter_coordonees_au_chemin_au_dernier_voisin(current.col, current.line, res);


    // verifier les 4 directions
    if(current.line-1 >= 0 && !Case_in_chemin(current.col, current.line-1, res) && l[current.col][current.line-1] != MUR && l[current.col][current.line-1] !=  VISITE) // left
        rec_find(l, res, (Case){current.col, current.line-1}, end);
    if(current.col - 1 >= 0 && !Case_in_chemin(current.col-1, current.line, res) && l[current.col-1][current.line] != MUR && l[current.col-1][current.line] !=  VISITE) // up
        rec_find(l, res, (Case){current.col-1, current.line}, end);   
    if(current.line+1 < COLS && !Case_in_chemin(current.col, current.line+1, res) && l[current.col][current.line+1] != MUR && l[current.col][current.line+1] !=  VISITE) // right
        rec_find(l, res, (Case){current.col, current.line+1}, end);
    if(current.col+1 < LINES && !Case_in_chemin(current.col+1, current.line, res) && l[current.col+1][current.line] != MUR && l[current.col+1][current.line] !=  VISITE) // down
        rec_find(l, res, (Case){current.col+1, current.line}, end);
}


Case* solve_labyrinth(laby l, int cols, int lines){
    // trouvons le depart
    Case start;
    for(int i=0; i< lines; i++)
        for(int j=0; j< cols; j++)
            if(l[i][j] == ENTREE){
                start.col = i;
                start.line = j;
                //arret des boucles
                i = lines;
                j = cols;
            }

    // trouvons l'arivee
    Case end;
    for(int i=0; i< lines; i++)
        for(int j=0; j< cols; j++)
            if(l[i][j] == EXIT){
                end.col = i;
                end.line = j;
                //arret des boucles
                i = lines;
                j = cols;
            }

    chemin reponse = malloc(sizeof(chemin) * CHEMIN_LENGTH);
    
    for(int i = 0; i < CHEMIN_LENGTH; i++)
        reponse[i] = (Case){-1, -1};
    
    rec_find(l, reponse, start, end);
    //remettre la case depart : 

    l[start.col][start.line] = ENTREE;
    // nettoyer la reponse si necessaire : 
    if(!check_solution(l, reponse))
        nettoyer_chemin(reponse);

    return reponse;
}

void print_Case(Case s){
    printf("%d %d\n", s.col, s.line);
}

void print_labyrinth(laby l, int cols, int lines){
    //print upper indexs : 
    for(int i = 0 ; i < cols ; i++)
        printf("%d", i%10);
    printf("|+\n");
    for(int i=0; i < lines; i++){
        for(int j=0; j < cols; j++)
            switch(l[i][j]){
                case MUR:
                    printf("#");
                    break;
                case WAY:
                    printf(" ");
                    break;
                case ENTREE:
                    printf("+");
                    break;
                case EXIT:
                    printf("-");
                    break;
                case VISITE:
                    printf(" ");
                    break;
                default:
                    printf("%c", l[i][j]);
                    break;
            }
        printf("|%d\n", i);
    }
}




int check_solution(laby l, Case*Case_tab){
    //verifier ENTREE
    if(l[Case_tab[0].col][Case_tab[0].line] != ENTREE)
        return 0; //false
    int index_end;
    for(index_end = 0; index_end < CHEMIN_LENGTH-2 ; index_end++)
        if(Case_tab[index_end+1].col == -1 && Case_tab[index_end+1].line == -1)
            break;

    if(index_end == CHEMIN_LENGTH - 1)
        return 0; // pas de fin de chemin

    //verifier que les cases sont voisines
    for(int i = 1; i <= index_end ; i++){
        if(!sont_voisines(Case_tab[i], Case_tab[i-1]))
            return 0;
    }

    return 1; // tous tests passes
}

void free_labyrinth(laby*l, int cols, int lines){
    if(*l == NULL)
        return; // rien a faire

    for(int i = 0 ; i < lines ; i++)
        if((*l)[i] != NULL)
            free((*l)[i]);
    if(*l != NULL)
        free(*l);
    *l = NULL;
}

laby creer_labyrinth(int cols, int lines){
    char* command = malloc(128);
    sprintf(command, "python3 generateur.py %d %d > out.txt",lines, cols);
    system(command);
    free(command);
    FILE* out = fopen("out.txt", "r");
    if (out == NULL) {
        printf("Failed to open file.\n");
    }
    laby current_laby = (laby)malloc(lines * sizeof(int *));
    for (int i = 0; i < lines; i++) {
        current_laby[i] = (int *)malloc(cols * sizeof(int));
    }

    int c;

    for (int i = 0; i < lines; i++) {
        for (int j = 0; j < cols; j++) {
            c = fgetc(out);
            if (c != '\n') {
                current_laby[i][j] = c - '0';
            }
        }
        fgetc(out); // ne pas prendre en compte le caracter "newline"
    }
    
    fclose(out);
    return current_laby;
}



int abs(int x){return x < 0 ? -x : x;}
int sont_voisines(Case a, Case b){return (a.col == b.col && abs(a.line - b.line) == 1) || (a.line == b.line && abs(a.col - b.col) == 1);}
int cases_egales(Case a, Case b){return a.col == b.col && a.line == b.line;}