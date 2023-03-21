#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#define UNUSED (-1)
#define MUR 0
#define WAY 1
#define ENTREE 2
#define EXIT 3
#define VISITE (-124)
#define END_SIGNAL (-123)
#define NB_THREAD 1
#define NB_THREAD_TOATL (20*NB_THREAD) // imaginons que le nb max de threads est 20 fois le nombre max de threads simultanes

#define CHEMIN_LENGTH (50)


typedef struct{
    int**m;
    int lignes;
    int cols;
} Laby;

typedef struct{
    int col;
    int line;
} Case;


typedef Case* chemin;

typedef struct {
    pthread_t id;
    chemin path;
} Thread_chemin;

typedef struct {
    Laby*l;
    chemin*res;         // liste des chemins effectués par les threads actuels
    pthread_t*threads;  // les des threads actuels
    pthread_t*threads_history; // liste totale de tous les threads ayant existes
    Case end;
    int * fini;
} Thread_args;


int nombre_ways(int col, int line);
void print(const char * msg);
Laby creer_labyrinth(int cols, int lines);
int check_solution(Laby, chemin);
void free_labyrinth(Laby*);
void print_labyrinth(Laby);
chemin solve_labyrinth(Laby);
chemin solve_labyrinth_threads(Laby l);
void rec_find(Laby, chemin res, Case start, Case end);
void rec_find_thread();
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