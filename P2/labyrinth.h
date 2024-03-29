#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

// #defines modifiables par l'utilisateur : 
#define NB_THREAD 2
#define NB_THREAD_TOTAL (20*NB_THREAD) // imaginons que le nb max de threads est 20 fois le nombre max de threads simultanes
#define CHEMIN_LENGTH (10000)

// #defines a ne pas changer
#define UNUSED (-1)
#define MUR 0
#define LIBRE 1
#define ENTREE 2
#define SORTIE 3
#define VISITE (-124) 
#define END_SIGNAL (-123)
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

/// @brief affiche pour chaque thread de 'global_args' son pthread_t et le chemin associe
void print_ids();

/// @param thread_index l'indice du thread pour lequel on veut une reponse
/// @return la derniere case ajoute a son chemin (non nulle)
int indiceDeDerniereCase(int thread_index);

/// @brief ajoute l'indentifiant renseigne dans le vecteur threads_history de global_args, ca servira lors du join 
/// @param thread_id identifiant a ajouter
void ajouter_dans_historique(pthread_t thread_id);

/// @brief resoud un labyrinth avec des threads (ne fonction de #define NB_THREAD, si NB_THREAD <= 1, resolution recursive, sinon resolution recursive avec des threads)
/// @param l le labyrinth a resoudre, avec 2 pour entree et 3 pour sortie
/// @return un vecteur de cases correspondant a l'alternance correcte des cases menant de l'entree a la sortie. les cases {-1, -1} ne sont pas a prendre en compte
chemin resoudre_avec_threads(Laby l);

/// @brief trouve l'indice du thread en fonction de son pthread_id et du champ threads de global_args. identitifiant obtenu avec la fonction pthread_self()
/// @return l'indice du thread
int get_thread_index();

/// @brief cherche une place de libre dans le champ threads de global_args
/// @return -1 si aucune place, sinon retourne l'indice disponible
int indiceDunePlaceLibre();

/// @brief affiche un message precede du pthread_t obtenu avec la fonction pthread_self() et utilise un mutex (acces_out) pour ecrire tout d'un coup dans le terminal
/// @param msg message a ecrire
void print(const char * msg);

/// @brief copie dans le champ res de global_args
/// @param from origine
/// @param to destination
void copier_chemins(int from, int to);

/// @brief verifie s'il y a au moins une possibilite de mouvement pour une case precise sur les 4 cases alentoures (cherche une case differente de VISITE et MUR)
/// @param c la case a partir de laquelle on cherche
/// @return 0 si la case n'a pas d'autre voisins que des murs ou cases visitees, 1 sinon
int une_possibilites_de_mouvement(Case c);

/// @brief affiche la solution avec des lettres a-z-A-Z-a... dans le labyrinthe
/// @param l labyrinthe en question
/// @param c chemin a afficher
void print_solution(Laby l, chemin c);

/// @brief cherche a savoir, etant donne l'indice du thread si son chemin associe est dans un cul de sac
/// @param t_id indice du thread
/// @return 1 si dans un cul de sac, 0 s'il y a des possibilites de mouvement
int est_dans_un_cul_de_sac(int t_id);

/// @brief affiche le chemin dans le terminal jusqu'aux cases {-1, -1}
/// @param c chemin a afficher
void print_chemin(chemin c);

/// @brief ajoute la case des qu'elle est voisine avec une case du chemin, effectue avec une boucle descendante. Toutes les cases d'apres seront remplaces par {-1, -1} pour permettre au backtrack de bien s'effectuer
/// @param col colonne de la case a ajouter
/// @param line ligne de la case a ajouter
/// @param c chemin dans lequel ajouter la case
/// @return 1 si la case a bien ete ajoutee, 0 s'il y a eu une erreur
int ajouter_coord_et_nettoyer_apres(int col, int line, chemin c);

/// @param col colonne de la case a tester
/// @param line ligne de la case a tester
/// @param c chemin dans lequel on teste 
/// @return 1 si la case est presente dans le chemin (compare les valeurs)
int caseDansChemin(int col, int line, chemin c);

/// @brief fonction recursive utilisant les threads seulement si NB_THREAD > 1. Sinon, la fonction rec_find sera appelée
void recursivite_thread();

/// @brief cree un labyrinth a partir d'un script python
/// @param cols nombre de colonnes souhaitees (> 3)
/// @param lines nombre de lignes souhaitees (> 3)
/// @return un labyrinthe (int**)
Laby creer_labyrinth(int cols, int lines);

/// @return |x|
int abso(int x);

/// @brief calcules si les deux cases sont voisines (les voisins en diagonale ne sont pas comptabilises)
/// @return 1 si les cases sont voisines, 0 sinon. si egalite : 0
int sont_voisines(Case a, Case b);

/// @brief calcules si deux cases sont égales (comparaison de leurs champs)
/// @return 1 si egalite, 0 sinon
int cases_egales(Case a, Case b);

/// @brief affiche une case dans le terminal
/// @param c case a afficher
void print_Case(Case);

/// @param l labyrinth pour lequel on cherche ENTREE (= 2)
/// @return la case correspondant a l'entree
Case trouver_entree(Laby l);

/// @param l labyrinth pour lequel on cherche SORTIE (= 2)
/// @return la case correspondant a la sortie
Case trouver_sortie(Laby l);

/// @brief desalloue la memoire alloue pour un labyrinthe (int**)
/// @param l adresse du labyrinthe
void free_labyrinth(Laby*);

/// @brief verifies qu'une solution est correcte (elle va bien du depart a l'arrivee et chaque cases sont voisines)
/// @param l labyrinthe concerne
/// @param c chemin a evaluer
/// @return 1 si solution correcte, 0 sinon
int verifier_solution(Laby, chemin);

/// @brief affiche le labyrinthe d'une maniere visuelle, avec '#', ' '...
/// @param  l le labyrinthe a afficher
void print_labyrinth(Laby);

/// @brief traitement paliatif au chemin trouve par la recusivite pure car il y a parfois des restes de backtrack dans le chemin reponse
/// @param c chemin a nettoyer
void nettoyer_chemin(chemin c);

/// @brief fonction recursive simple pour trouver une solution au labyrinth
/// @param l labyrinth probleme
/// @param res vecteur de case, reponse
/// @param current case actuelle
/// @param end case finale (cible)
void recursivite_simple(Laby l, chemin res, Case current, Case end);

/// @brief lance la recursivite simple pour resoudre le labyrinthe et renvoit son resultat
/// @param l labyrinthe a resoudre
/// @return un vecteur de case, chemin reponse
chemin resoudre_recursivement(Laby l);

/// @brief alloue la stucture partagee servant a la resolution recursive avec des threads du labyrinthe
/// @param l addresse du labyrinthe
/// @param start case de depart
/// @param end case de fin
void allouer_arguments(Laby *l, Case start, Case end);

/// @brief libere la memoire de la structure partagee
void free_arguments();

/// @brief remet a zero les informations liees a un thread, utile pour le terminer
/// @param thread_indice indice du thread a terminer
/// @param derniere_case indice de la derniere case de ce thread
void reset_id_et_chemin(int thread_indice, int derniere_case);