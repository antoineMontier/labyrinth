#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define UNUSED (-1)
#define MUR 0
#define WAY 1
#define ENTREE 2
#define EXIT 3
#define VISITE (-124)
#define END_SIGNAL (-123)
#define NB_THREAD 8

#define HAUT (-1111)
#define GAUCHE (-2222)
#define DROITE (-3333)
#define BAS (-4444)

#define CHEMIN_LENGTH (500)


typedef struct{
    int**m;
    int lignes;
    int cols;
} Laby;

typedef struct{
    int col;
    int line;
} Case;

typedef struct{
    pthread_t* ids;
    int* used;
    pthread_t** sons;
} Thread_manager;

typedef Case* chemin;

typedef struct {
    Laby*l;
    chemin res;
    Case* current;
    Case* end;
    Thread_manager*tm;
    pthread_t father;
    int*solution_trouvee;
} Thread_args;

int actual_ind(Thread_manager *t)
void end_actual_thread_signal_without_cancel(Thread_manager *t);
void end_actual_thread_signal(Thread_manager *t);
Thread_manager creer_threads();
void free_threads(Thread_manager *t);
Laby creer_labyrinth(int cols, int lines);
int check_solution(Laby, chemin);
void free_labyrinth(Laby*);
void print_labyrinth(Laby);
chemin solve_labyrinth(Laby);
chemin solve_labyrinth_threads(Laby l);
void rec_find(Laby, chemin res, Case start, Case end);
chemin rec_find_thread(void* th_args);
void print_Case(Case);
int Case_in_chemin(int col, int line, chemin c);
void ajouter_coordonees_au_chemin_au_dernier_voisin(int col, int line, chemin c);
void print_chemin(chemin c);
void print_solution(Laby l, chemin c);
int abso(int x);
int sont_voisines(Case a, Case b); // ne fonctionne pas en diagonale
int cases_egales(Case a, Case b);
int ajouter_au_dernier_voisin(chemin c, Case a_ajouter);
void nettoyer_chemin(chemin c); // le probleme est que parfois les cases se suivent bien sauf entre l'avant derniere et la dernier (=arrivee) il s'intercale des cases deja visitees