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
#define NB_THREAD 2
#define NB_THREAD_TOATL (20*NB_THREAD) // imaginons que le nb max de threads est 20 fois le nombre max de threads simultanes

#define CHEMIN_LENGTH (2000)
#define CASE_NULLE ((Case) {-1, -1})


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
    Laby*l;                     /// labyrinth
    chemin*res;                 /// liste des chemins effectués par les threads actuels
    pthread_t*threads;          /// les id des threads actuels
    pthread_t*threads_history;  /// liste totale de tous les threads ayant existes
    Case end;                   /// case finale
} Thread_args;

void print_ids();
int getLastCaseIndex(int thread_index);
void ajouter_dans_historique(pthread_t thread_id);
chemin solve_labyrinth_threads(Laby l);
int get_thread_num();
int get_first_room_for_new_thread();
void print(const char * msg);
void copier_chemins(int from, int to);
int une_possibilites_de_mouvement(Case c);
void print_solution(Laby l, chemin c);
int est_dans_un_cul_de_sac(int t_id);
void print_chemin(chemin c);
int ajouter_coord_et_nettoyer_apres(int col, int line, chemin c);
int case_in_chemin(int col, int line, chemin c);
void rec_find_thread();
Laby creer_labyrinth(int cols, int lines);
int abso(int x);
int sont_voisines(Case a, Case b); // ne fonctionne pas en diagonale
int cases_egales(Case a, Case b);
void print_Case(Case);
Case trouver_entree(Laby l);
Case trouver_sortie(Laby l);
void free_labyrinth(Laby*);
int check_solution(Laby, chemin);
void print_labyrinth(Laby);
void print_raw_labyrinth(Laby l);
int ajouter_au_dernier_voisin(chemin c, Case a_ajouter);
void nettoyer_chemin(chemin c);
void rec_find(Laby l, chemin res, Case current, Case end);
chemin solve_labyrinth(Laby l);