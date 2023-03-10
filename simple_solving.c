#include "labyrinth.h"

int main(){
    Laby l = creer_labyrinth(11, 11);
    print_labyrinth(l);
    chemin ans = solve_labyrinth(l);
    print_chemin(ans);
    if(check_solution(l, ans))
        printf("Solution OK\n");
    print_solution(l, ans);
    free(ans);
    free_labyrinth(&l);
    return 0;
}