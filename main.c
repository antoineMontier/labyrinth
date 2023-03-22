#include "labyrinth.h"

extern Thread_args *global_args;


/*question : In c programming, imagine the main thread creates a son thread that also creates another son thread (linear). If the 1st son die, is it possible for the main thread to join the last son, knowing its pthread_t id ?*/
/*question au prof : esce que c'est un probleme si on lance le programme en thread : CAD le thread main ne participe pas a la resolution... si oui, NB_THREAD correspondrait au nombre max de threads participant a la resolution du probleme*/
/*bug persistant : si lors d'une recursivite quasi pure, le thread s=fait un backtrack, il voudra lancer plus tard une recursivite depuis un lieu dont il se sera retire, perte d'information entre le backtrack et le nv lancement*/
int main(){
    Laby l = creer_labyrinth(9, 9);
    print_labyrinth(l);
    chemin ans = solve_labyrinth_threads(l);
    printf("before print_chemin\n");
    print_chemin(ans);
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