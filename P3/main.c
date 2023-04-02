#include "labyrinth.h"
/* 
Fonctions utilisateur : 
    print_*
    creer_labyrinth
    trouver_*
    free_labyrinth
    verifier_solution
    course_de_process

voir labyrinth.h pour les modifications sur quelques #define

Toutes les fonctions sont documentees
*/
int main(){
    Laby l = creer_labyrinth(60, 80);
    // print_labyrinth(l);
    chemin* res = course_de_process(l, 1);

    if(verifier_solution(l, res[0], trouver_entree_1(l), trouver_porte(l), trouver_sortie_1(l)))    printf("solution correcte entre E1 et S1\n");
    else                                                                                            printf("solution INCORRECTE entre E1 et S1\n");

    if(verifier_solution(l, res[1], trouver_entree_2(l), trouver_porte(l), trouver_sortie_2(l)))    printf("solution correcte entre E2 et S2\n");
    else                                                                                            printf("solution INCORRECTE entre E2 et S2\n");

    free(res[0]); free(res[1]); free(res);
    free_labyrinth(&l);
    return 0;
}