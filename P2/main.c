#include "labyrinth.h"

/*question : In c programming, imagine the main thread creates a son thread that also creates another son thread (linear). If the 1st son die, is it possible for the main thread to join the last son, knowing its pthread_t id ?*/
/*plusieurs sorties : transformer la sortie qu'on vient de trouver en une entree : ENTREE > PORTE > SORTIE 2 threads s'attendent a la porte et partent chacun vers leur sortie*/


int main(){
    Laby l = creer_labyrinth(7, 11);
    print_labyrinth(l);
    chemin ans = solve_labyrinth_threads(l);
    printf("before print_chemin\n");
    //print_chemin(ans);
    if(check_solution(l, ans))
        printf("Solution OK\n");
    else
        printf("Solution NOT ok\n");
    print_solution(l, ans);
    printf("before free\n");
    free(ans);
    free_labyrinth(&l);
    printf("after free\n");

    return 0;
}