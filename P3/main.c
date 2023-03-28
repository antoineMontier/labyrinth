#include "labyrinth.h"

/*question : In c programming, imagine the main thread creates a son thread that also creates another son thread (linear). If the 1st son die, is it possible for the main thread to join the last son, knowing its pthread_t id ?*/
/*plusieurs sorties : transformer la sortie qu'on vient de trouver en une entree : ENTREE > PORTE > SORTIE 2 threads s'attendent a la porte et partent chacun vers leur sortie*/
/*todo : es ce que l'historique de pids est utile ??? +++ retirer les "down", right"...*/
/*
100 mains :
./main && ./main && ./main ./main && ./main && ./main ./main && ./main && ./main && ./main && ./main && ./main && ./main ./main && ./main && ./main ./main && ./main && ./main && ./main && ./main && ./main && ./main ./main && ./main && ./main ./main && ./main && ./main && ./main && ./main && ./main && ./main ./main && ./main && ./main ./main && ./main && ./main && ./main && ./main && ./main && ./main ./main && ./main && ./main ./main && ./main && ./main && ./main && ./main && ./main && ./main ./main && ./main && ./main ./main && ./main && ./main && ./main && ./main && ./main && ./main ./main && ./main && ./main ./main && ./main && ./main && ./main && ./main && ./main && ./main ./main && ./main && ./main ./main && ./main && ./main && ./main && ./main && ./main && ./main ./main && ./main && ./main ./main && ./main && ./main && ./main && ./main && ./main && ./main ./main && ./main && ./main ./main && ./main && ./main && ./main
*/
int main(){
    Laby l = creer_labyrinth(10, 10);
    print_labyrinth(l);
    chemin* res = P3(l);//solve_labyrinth_threads(l, (Case)trouver_porte(l), (Case)trouver_sortie_1(l));
    //print_solution(l, res);
    //free(res);
    free(res[0]);
    free(res[1]);
    free(res);
    free_labyrinth(&l);
    return 0;
}
