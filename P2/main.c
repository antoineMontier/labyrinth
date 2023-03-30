#include "labyrinth.h"

/*
100 mains :
./main && ./main && ./main ./main && ./main && ./main ./main && ./main && ./main && ./main && ./main && ./main && ./main ./main && ./main && ./main ./main && ./main && ./main && ./main && ./main && ./main && ./main ./main && ./main && ./main ./main && ./main && ./main && ./main && ./main && ./main && ./main ./main && ./main && ./main ./main && ./main && ./main && ./main && ./main && ./main && ./main ./main && ./main && ./main ./main && ./main && ./main && ./main && ./main && ./main && ./main ./main && ./main && ./main ./main && ./main && ./main && ./main && ./main && ./main && ./main ./main && ./main && ./main ./main && ./main && ./main && ./main && ./main && ./main && ./main ./main && ./main && ./main ./main && ./main && ./main && ./main && ./main && ./main && ./main ./main && ./main && ./main ./main && ./main && ./main && ./main && ./main && ./main && ./main ./main && ./main && ./main ./main && ./main && ./main && ./main
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
