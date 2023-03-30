#include "labyrinth.h"

/*question : In c programming, imagine the main thread creates a son thread that also creates another son thread (linear). If the 1st son die, is it possible for the main thread to join the last son, knowing its pthread_t id ?*/
/*plusieurs sorties : transformer la sortie qu'on vient de trouver en une entree : ENTREE > PORTE > SORTIE 2 threads s'attendent a la porte et partent chacun vers leur sortie*/
/*todo : es ce que l'historique de pids est utile ??? +++ retirer les "down", right"...*/
/*
100 mains :
./main && ./main && ./main ./main && ./main && ./main ./main && ./main && ./main && ./main && ./main && ./main && ./main ./main && ./main && ./main ./main && ./main && ./main && ./main && ./main && ./main && ./main ./main && ./main && ./main ./main && ./main && ./main && ./main && ./main && ./main && ./main ./main && ./main && ./main ./main && ./main && ./main && ./main && ./main && ./main && ./main ./main && ./main && ./main ./main && ./main && ./main && ./main && ./main && ./main && ./main ./main && ./main && ./main ./main && ./main && ./main && ./main && ./main && ./main && ./main ./main && ./main && ./main ./main && ./main && ./main && ./main && ./main && ./main && ./main ./main && ./main && ./main ./main && ./main && ./main && ./main && ./main && ./main && ./main ./main && ./main && ./main ./main && ./main && ./main && ./main && ./main && ./main && ./main ./main && ./main && ./main ./main && ./main && ./main && ./main
*/
int main(){
    Laby l = creer_labyrinth(60, 80);
    // print_labyrinth(l);
    chemin* res = P3(l);
    nettoie_matrice(l);
    //print_labyrinth(l);

    if(check_solution(l, res[0], trouver_entree_1(l), trouver_porte(l), trouver_sortie_1(l)))
        printf("solution correcte entre E1 et S1\n");
    else
        printf("solution INCORRECTE entre E1 et S1\n");

    if(check_solution(l, res[1], trouver_entree_2(l), trouver_porte(l), trouver_sortie_2(l)))
        printf("solution correcte entre E2 et S2\n");
    else
        printf("solution INCORRECTE entre E2 et S2\n");
    printf("================================================================\n");
    free(res[0]);
    free(res[1]);
    free(res);
    free_labyrinth(&l);
    return 0;
}
