#include "labyrinth.h"

int main(){
    Laby l = creer_labyrinth(11, 11);
    chemin ans = solve_labyrinth_threads(l);
    if(check_solution(l, ans))
        printf("Solution OK\n");
    print_solution(l, ans);
    free(ans);
    free_labyrinth(&l);
    return 0;
}