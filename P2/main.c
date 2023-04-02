#include "labyrinth.h"

/*
Fonctions utilisateur :
    resoudre_avec_threads
    creer_labyrinth
    trouver_*
    print_*
    free_labyrinth
    verifier_solution
    resoudre_recursivement

voir labyrinth.h pour les modifications sur quelques #define

Toutes les fonctions sont documentees
*/
int main(){
    Laby l = creer_labyrinth(80, 20);
    print_labyrinth(l); 
    chemin ans = resoudre_avec_threads(l);
    printf("\n\n");
    if(verifier_solution(l, ans))
        printf("Solution VRAIE\n");
    else
        printf("Solution FAUSSE\n");
    print_chemin(ans);
    printf("\n\n");
    print_solution(l, ans);
    free(ans);
    free_labyrinth(&l);
    return 0;
}
