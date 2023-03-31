#include "labyrinth.h"

/*
Fonctions utilisateur :
    resoudre_avec_threads
    recursivite_thread
    creer_labyrinth
    trouver_*
    print_*
    free_labyrinth
    verifier_solution
    resoudre_recursivement

voir labyrinth.h pour les modifications sur quelques #define
*/
int main(){
    Laby l = creer_labyrinth(10, 20);
    //print_labyrinth(l); 
    chemin ans = resoudre_avec_threads(l);
    if(verifier_solution(l, ans))
        printf("Solution VRAIE\n");
    else
        printf("Solution FAUSSE\n");
    //print_chemin(ans);
    //print_solution(l, ans);
    free(ans);
    free_labyrinth(&l);
    return 0;
}
