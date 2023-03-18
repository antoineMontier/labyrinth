#include "labyrinth.h"

extern Thread_args *global_args;

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