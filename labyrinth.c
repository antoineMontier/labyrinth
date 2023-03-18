#include "labyrinth.h"  

Thread_args * global_args;

void print_sols(){
    for(int i = 0 ; i < NB_THREAD ; i++){
        printf("%ld\t", global_args->threads[i]);
        print_chemin(global_args->res[i]);
    }
}

void print_ids(){
    printf("\n\n=================Threads _ids :\n");
    for(int i =  0; i < NB_THREAD ; ++i){
        printf("%ld -- ", global_args->threads[i]);
        print_chemin(global_args->res[i]);
    }
    printf("=================\n\n");
}

int getLastCaseIndex(int thread_index){
    if(thread_index == -1){
        printf("getLastCaseIndex error, thread index is -1\n");
        return -1;
    }
    if(cases_egales(global_args->res[thread_index][0], (Case){UNUSED, UNUSED}))
        return -1;
    for(int i = 1 ; i < CHEMIN_LENGTH ; ++i)
        if(cases_egales(global_args->res[thread_index][i], (Case){UNUSED, UNUSED}))
            return i-1;
    return CHEMIN_LENGTH;
}

chemin solve_labyrinth_threads(Laby l){
    // trouvons le depart
    Case start = (Case){UNUSED, UNUSED};
    for(int i=0; i< l.cols; i++)
        for(int j=0; j< l.lignes; j++)
            if(l.m[i][j] == ENTREE){
                start.col = i;
                start.line = j;
                //arret des boucles
                i = l.cols;
                j = l.lignes;
            }

    // trouvons l'arivee
    Case end = (Case){UNUSED, UNUSED};
    for(int i=0; i< l.lignes; i++)
        for(int j=0; j< l.cols; j++)
            if(l.m[i][j] == EXIT){
                end.col = i;
                end.line = j;
                //arret des boucles
                i = l.cols;
                j = l.lignes;
            }

    printf("start: c: %d ; l: %d\n", start.col, start.line);

    if(cases_egales(start, (Case){-1, -1}) || cases_egales(end, (Case){-1, -1})){
        printf("Impossible de trouver l'entree ou la sortie\n");
        exit(1);
    }

    // lancer la recursivite avec un thread !!!
    global_args = malloc(sizeof(Thread_args));
    global_args->l = &l;
    global_args->end = end;
    global_args->fini = malloc(sizeof(int));
    *(global_args->fini) = 0;
    global_args->res = malloc(NB_THREAD*sizeof(chemin));
    for(int i = 0 ; i < NB_THREAD ; i++){
        global_args->res[i] = malloc(CHEMIN_LENGTH*sizeof(chemin));
        for(int j = 1 ; j < CHEMIN_LENGTH ; j++)
            global_args->res[i][j] = (Case){UNUSED, UNUSED};
        global_args->res[i][0] = (Case){start.col, start.line};
    }
    global_args->threads = malloc(NB_THREAD*sizeof(pthread_t));
    for(int i = 0 ; i < NB_THREAD ; i++)
        global_args->threads[i] = 0;

    printf("avant le lancement de la recursivite\n");
    pthread_create(&(global_args->threads[0]), NULL, (void*)rec_find_thread,NULL);
    //while(*(global_args->fini) == 0)print_sols();
    printf("apres le lancement de la recursivite\n");

    for(int i = 0 ; i < NB_THREAD ; ++i)
        if(global_args->threads[i] != 0){
            pthread_join(global_args->threads[i], NULL);
            global_args->threads[i] = 0;
        }
    printf("apres le join\n");
    print_ids();


    // lire le chemin reponse dans le tableau de chemin : 

    chemin reponse_finale = malloc(CHEMIN_LENGTH * sizeof(Case));
    for(int i = 0 ; i < NB_THREAD ; ++i){
        if(cases_egales(global_args->res[i][getLastCaseIndex(i)], end)) { // chemin directement trouvé 
            for(int j = 0 ; j < CHEMIN_LENGTH ; ++j)
                reponse_finale[j] = global_args->res[i][j];
            i = NB_THREAD; // stop boucle
        }else if(sont_voisines(global_args->res[i][getLastCaseIndex(i)], end)){ // chemin trouve mais il manque la derniere case
            for(int j = 0 ; j < CHEMIN_LENGTH ; ++j)
                reponse_finale[j] = global_args->res[i][j];
            ajouter_au_dernier_voisin(reponse_finale, end); // ajouter la derniere case a la main
            i = NB_THREAD; // stop boucle
        }
    }

    print_sols();
            

    // rendre l'espace utilisé
    for(int i = 0 ; i < NB_THREAD ; ++i)
        free(global_args->res[i]);
    free(global_args->threads);
    free(global_args->res);
    free(global_args->fini);
    free(global_args);


    
    //remettre la case depart : 
    l.m[start.col][start.line] = ENTREE;

    // nettoyer la reponse si necessaire : 
    // if(!check_solution(l, reponse)) nettoyer_chemin(reponse);

    return reponse_finale;
}

int get_thread_num(){
    for(int i = 0 ; i < NB_THREAD ; ++i)
        if(global_args->threads[i] == pthread_self()){
            return i;
        }
    return -1;
}

int get_first_room_for_new_thread(){
    for(int i = 0 ; i < NB_THREAD ; ++i)
        if(global_args->threads[i] == 0)
            return i;
    return NB_THREAD;
}

void stopper_thread_et_reset_chemin(int thread_index){
    // while (pthread_mutex_trylock(&mutex) == 0){
    //     printf("mutex is already locked, impossible to stop thread\n");
    // }
    for(int i = 0 ; i < CHEMIN_LENGTH ; ++i)
        global_args->res[thread_index][i] = (Case){UNUSED, UNUSED};
    global_args->threads[thread_index] = 0;
    pthread_exit(NULL);
}





void rec_find_thread(){
    // print_sols();
    pthread_mutex_lock(&mem_reader_mutex);
    int thread_num = get_thread_num();
    if(thread_num == -1){
        printf("%ld:\timpossible de connaitre l'indice du thread %ld\n", pthread_self(), pthread_self());
        /*sleep(10);
        if((thread_num = get_thread_num()) == -1){
            printf("%ld:\timpossible de connaitre l'indice apres le 2nd essai, fin de programme \n", pthread_self());
            exit(1);
        }
        printf("%ld:\tresolu\n", pthread_self());*/
    }

    int case_ind = getLastCaseIndex(thread_num);
    if(case_ind == -1)
        printf("%ld:\timpossible de connaitre l'indice de la case courante du thread %ld\n", pthread_self(), pthread_self());
    if(case_ind == CHEMIN_LENGTH)
        printf("%ld:\tDans le thread %ld, il ne semble pas y avoir de case courante\n", pthread_self(), pthread_self());
    
    if (global_args == NULL) {
        printf("global_args is NULL\n");
        exit(1);
    }

    if (global_args->res[thread_num] == NULL) {
        printf("global_args->res[%d] is NULL\n", thread_num);
        exit(1);
    }
    int actual_line = global_args->res[thread_num][case_ind].line;
    int acutal_col = global_args->res[thread_num][case_ind].col;
    pthread_mutex_unlock(&mem_reader_mutex);

    //printf("%ld:\tlocated on slot c: %d | l: %d\t\t", pthread_self(), acutal_col, actual_line);
    pthread_mutex_lock(&print_mutex);
    print_ids();
    pthread_mutex_unlock(&print_mutex);
    // printf("%ld:\taaaa\n", pthread_self());
    if(*(global_args->fini)){ // chemin trouvé par un autre thread
        printf("%ld:\tje m'arrete car la solution est trouvee, fini = 1\n", pthread_self());
        stopper_thread_et_reset_chemin(thread_num);
    }
    //printf("%ld:\tbbbbb\n", pthread_self());
    //continue here 
    if(cases_egales((global_args->res[thread_num][case_ind]), (global_args->end))){ // le thread actuel a trouvé la bonne réponse
        printf("%ld:\tje m'arrete car je suis sur la case reponse\n", pthread_self());
        *(global_args->fini) = 1; // communiquer aux autres threads qu'il faut qu'ils s'arretent
        pthread_exit(NULL); // stopper le thread reponse ici
    }   

    //printf("%ld:\tcccc\n", pthread_self());
    // marquer la case comme visitée : 
    global_args->l->m[acutal_col][actual_line] = VISITE;
    //printf("%ld:\tdddd\n", pthread_self());

    /*
    // ajouter la case dans le chemin du thread actuel
    ajouter_au_dernier_voisin(t->res[thread_num], *(t->current));
    */

    // regarder dans les 4 directions

    int up = 0, left = 0, right = 0, down = 0, nb_direction = 0;
    if(actual_line-1 >= 0                       && !Case_in_chemin(acutal_col, actual_line-1, global_args->res[thread_num])     && global_args->l->m[acutal_col][actual_line-1] != MUR  && global_args->l->m[acutal_col][actual_line-1] !=  VISITE) up = 1;
    if(acutal_col - 1 >= 0                      && !Case_in_chemin(acutal_col-1, actual_line, global_args->res[thread_num])     && global_args->l->m[acutal_col-1][actual_line] != MUR  && global_args->l->m[acutal_col-1][actual_line] !=  VISITE) left = 1;  
    if(actual_line+1 < global_args->l->cols     && !Case_in_chemin(acutal_col, actual_line+1, global_args->res[thread_num])     && global_args->l->m[acutal_col][actual_line+1] != MUR  && global_args->l->m[acutal_col][actual_line+1] !=  VISITE) down = 1;
    if(acutal_col+1 < global_args->l->lignes    && !Case_in_chemin(acutal_col+1, actual_line, global_args->res[thread_num])     && global_args->l->m[acutal_col+1][actual_line] != MUR  && global_args->l->m[acutal_col+1][actual_line] !=  VISITE) right = 1;

    // ================ 
    if(left) ++nb_direction;
    if(up) ++nb_direction;
    if(right) ++nb_direction;
    if(down) ++nb_direction;
    // ================



    /*
    if(!left && !up && !right && !down){ // supprimer le chemin fait par le thread en marquant les cases comme inutilisees
        for(int i = 0 ; i < CHEMIN_LENGTH ; ++i)
            t->res[thread_num][0] = (Case){UNUSED, UNUSED};
        pthread_exit(NULL); // arreter le thread ici
    }
    */



    if(up){
        printf("%ld\t\t:\tup nb_dir = %d\n", pthread_self(), nb_direction); 
        nb_direction--;
        int nvl_place;
        if(!((nvl_place = get_first_room_for_new_thread()) == NB_THREAD || nb_direction == 0)){
            // lancer avec un thread
            //printf("%ld:\tcreating new thread\n", pthread_self()); fflush(stdout);
            //printf("..from %d %d   to up\n", acutal_col, actual_line);
            // copier le chemin parcouru avant de lancer le nouveau thread
            pthread_mutex_lock(&mutex);
            for(int k = 0; k < CHEMIN_LENGTH; ++k) // optimisation possible avec le check de {UNUSED,UNUSED}
                global_args->res[nvl_place][k] = global_args->res[thread_num][k];
            ajouter_coordonees_au_chemin_au_dernier_voisin(acutal_col, actual_line-1, global_args->res[nvl_place]); // ajouter le nouveau chemin
            pthread_create((pthread_t*)(global_args->threads + nvl_place), NULL, (void*)rec_find_thread, NULL); // lancer le nouveau thread
            printf("%ld:\t just created thread n°%ld On slot %d\n", pthread_self(), global_args->threads[nvl_place], nvl_place);
            pthread_mutex_unlock(&mutex);
        }else{
             // simple recusivité à lancer dans le thread actuel si une seule direction ou si le nb max de thread est atteint
            //printf("%ld:\tlaunching new rec function\n", pthread_self());fflush(stdout);
            //printf("..from %d %d   to up\n", acutal_col, actual_line);
            ajouter_coordonees_au_chemin_au_dernier_voisin(acutal_col, actual_line-1, global_args->res[thread_num]);
            rec_find_thread();
        }
    }if(left){
        printf("%ld:\t\t\tleft nb_dir = %d\n", pthread_self(), nb_direction); 
        nb_direction--;
        int nvl_place;
        if(!((nvl_place = get_first_room_for_new_thread()) == NB_THREAD || nb_direction == 0)){
            // lancer avec un thread
            //printf("%ld:\tcreating new thread\n", pthread_self()); fflush(stdout);
            //printf("..from %d %d   to left\n", acutal_col, actual_line);
            // copier le chemin parcouru avant de lancer le nouveau thread
            pthread_mutex_lock(&mutex);
            for(int k = 0; k < CHEMIN_LENGTH; ++k) // optimisation possible avec le check de {UNUSED,UNUSED}
                global_args->res[nvl_place][k] = global_args->res[thread_num][k];
            ajouter_coordonees_au_chemin_au_dernier_voisin(acutal_col-1, actual_line, global_args->res[nvl_place]); // ajouter le nouveau chemin
            pthread_create((pthread_t*)(global_args->threads + nvl_place), NULL, (void*)rec_find_thread, NULL); // lancer le nouveau thread
            printf("%ld:\t just created thread n°%ld On slot %d\n", pthread_self(), global_args->threads[nvl_place], nvl_place);
            pthread_mutex_unlock(&mutex);
        }else{
             // simple recusivité à lancer dans le thread actuel si une seule direction ou si le nb max de thread est atteint
            //printf("%ld:\tlaunching new rec function\n", pthread_self());fflush(stdout);
            //printf("..from %d %d   to left\n", acutal_col, actual_line);
            ajouter_coordonees_au_chemin_au_dernier_voisin(acutal_col-1, actual_line, global_args->res[thread_num]);
            rec_find_thread();
        }
    }if(down){
        printf("%ld:\t\t\tdown nb_dir = %d\n", pthread_self(), nb_direction); 
        nb_direction--;
        int nvl_place;
        if(!((nvl_place = get_first_room_for_new_thread()) == NB_THREAD || nb_direction == 0)){
            // lancer avec un thread
           // printf("%ld:\tcreating new thread\n", pthread_self()); fflush(stdout);
           // printf("..from %d %d   to down\n", acutal_col, actual_line);
            // copier le chemin parcouru avant de lancer le nouveau thread
            pthread_mutex_lock(&mutex);
            for(int k = 0; k < CHEMIN_LENGTH; ++k) // optimisation possible avec le check de {UNUSED,UNUSED}
                global_args->res[nvl_place][k] = global_args->res[thread_num][k];
            ajouter_coordonees_au_chemin_au_dernier_voisin(acutal_col, actual_line+1, global_args->res[nvl_place]); // ajouter le nouveau chemin
            pthread_create((pthread_t*)(global_args->threads + nvl_place), NULL, (void*)rec_find_thread, NULL); // lancer le nouveau thread
            printf("%ld:\t just created thread n°%ld On slot %d\n", pthread_self(), global_args->threads[nvl_place], nvl_place);
            pthread_mutex_unlock(&mutex);
        }else{
             // simple recusivité à lancer dans le thread actuel si une seule direction ou si le nb max de thread est atteint
            //printf("%ld:\tlaunching new rec function\n", pthread_self());fflush(stdout);
            //printf("..from %d %d   to down\n", acutal_col, actual_line);
            ajouter_coordonees_au_chemin_au_dernier_voisin(acutal_col, actual_line+1, global_args->res[thread_num]);
            rec_find_thread();
        }
    }if(right){
        printf("%ld:\t\t\tright nb_dir = %d\n", pthread_self(), nb_direction); 
        nb_direction--;
        int nvl_place;
        if(!((nvl_place = get_first_room_for_new_thread()) == NB_THREAD || nb_direction == 0)){
            // lancer avec un thread
            //printf("%ld:\tcreating new thread\n", pthread_self()); fflush(stdout);
            //printf("..from %d %d   to right\n", acutal_col, actual_line);
            // copier le chemin parcouru avant de lancer le nouveau thread
            pthread_mutex_lock(&mutex);
            for(int k = 0; k < CHEMIN_LENGTH; ++k) // optimisation possible avec le check de {UNUSED,UNUSED}
                global_args->res[nvl_place][k] = global_args->res[thread_num][k];
            ajouter_coordonees_au_chemin_au_dernier_voisin(acutal_col+1, actual_line, global_args->res[nvl_place]); // ajouter le nouveau chemin
            pthread_create((pthread_t*)(global_args->threads + nvl_place), NULL, (void*)rec_find_thread, NULL); // lancer le nouveau thread
            printf("%ld:\t just created thread n°%ld On slot %d\n", pthread_self(), global_args->threads[nvl_place], nvl_place);
            pthread_mutex_unlock(&mutex);
        }else{
             // simple recusivité à lancer dans le thread actuel si une seule direction ou si le nb max de thread est atteint
            //printf("%ld:\tlaunching new rec function\n", pthread_self());fflush(stdout);
            //printf("..from %d %d   to right\n", acutal_col, actual_line);
            ajouter_coordonees_au_chemin_au_dernier_voisin(acutal_col+1, actual_line, global_args->res[thread_num]);
            rec_find_thread();
        }
    }

    if(*(global_args->fini)){ // chemin trouvé par un autre thread
        printf("%ld:\tje m'arrete car la solution est trouvee, fini = 1\n", pthread_self());
        stopper_thread_et_reset_chemin(thread_num);
    }

    // pas sur de ce point ::
    // pthread_mutex_lock(&mutex);
    // stopper_thread_et_reset_chemin(thread_num);
    // pthread_mutex_unlock(&mutex);
}






void nettoyer_chemin(chemin c){
    // se placer à l'indice fin : 
    int ind_fin = CHEMIN_LENGTH-2;
    while(ind_fin >= 0 && c[ind_fin].col == UNUSED && c[ind_fin].line == UNUSED) ind_fin--;

    if(ind_fin == 0) // chemin n'a aucune case valable, rien à nettoyer
        return;
    
    // chercher le premier voisin en continuant de remonter vers le depart
    while(ind_fin >=0 && !sont_voisines(c[ind_fin], c[ind_fin-1])){
        c[ind_fin -1] = c[ind_fin];
        c[ind_fin] = (Case){UNUSED , UNUSED};
        --ind_fin;
    }
}


void print_solution(Laby l, chemin c){
    // ajouter des lettres dans le labyrinth pour montrer le chemin pris
    for(int i=1; i<CHEMIN_LENGTH-1 ; ++i){
        if(c[i+1].col == UNUSED && c[i+1].line == UNUSED)
            break;
        else
            l.m[c[i].col][c[i].line] = i < 27 ? 'a' + i - 1 : 'A' + i - 27; // affichage du chemin avec les characteres ascii a,b->z,A,B->Z
        }

    print_labyrinth(l);

    // retirer les lettres
    for(int i=1; i<CHEMIN_LENGTH ; ++i){
        if(c[i].col == UNUSED&& c[i].line ==UNUSED)
            break;
        else
            l.m[c[i].col][c[i].line] = WAY;
        }
}


void print_chemin(chemin c){
    printf("[");
    for(int i = 0; i < CHEMIN_LENGTH ; i++){
        if(c[i].col == UNUSED && c[i].line == UNUSED)
            break;
        printf(" %d;%d ", c[i].col, c[i].line);
    }
    printf("]\n");
}


void ajouter_coordonees_au_chemin_au_dernier_voisin(int col, int line, chemin c){
    if(c[CHEMIN_LENGTH-1].col == END_SIGNAL && c[CHEMIN_LENGTH-1].line == END_SIGNAL)
        return; // end signal : solution deja trouvee
    Case s = {col, line};
    // printf("about to add %d;%d\n", col, line  );
    for(int i = 0 ; i < CHEMIN_LENGTH; i++)
        if(c[i].col == UNUSED && c[i].line == UNUSED){
            if(i == 0){
                c[i] = s;
                return;
            }else
                ajouter_au_dernier_voisin(c, s);
    }
}

int ajouter_au_dernier_voisin(chemin c, Case a_ajouter){
    for(int i = CHEMIN_LENGTH-2; i >= 0; i--){
        if(!(c[i].col == UNUSED && c[i].line == UNUSED) && sont_voisines(c[i], a_ajouter)){// skipper toutes les cases à la fin de coordonnees{-1 ; -1}
            c[i+1] = a_ajouter;
            return 1; // case ajoutee
        }
    }
    return 0; // aucune case ajoutee
}


int Case_in_chemin(int col, int line, chemin c){
    for(int i=0; i<CHEMIN_LENGTH; i++)
        if(c[i].col == col && c[i].line == line)
            return 1;
    return 0;
}

void rec_find(Laby l, chemin res, Case current, Case end){

    if(res == NULL)
        fprintf(stderr, "chemin doit etre declare avec malloc\n");

    if(res[CHEMIN_LENGTH-1].col == END_SIGNAL && res[CHEMIN_LENGTH-1].line == END_SIGNAL) // check end_signal
        return; // une solution a deja ete trouvee
    
    if(cases_egales(current, end)){

        // ajouter à la main la derniere case dans le chemin :
        for(int i=0; i<CHEMIN_LENGTH; i++)
            if(res[i].col == UNUSED && res[i].line == UNUSED){
                res[i] = end;
                break;
            }
        res[CHEMIN_LENGTH-1] = (Case){END_SIGNAL, END_SIGNAL};
        return;
    }

    // marquer la case comme visitée : 
    l.m[current.col][current.line] = VISITE;

    //ajouter la case dans le chemin
    ajouter_coordonees_au_chemin_au_dernier_voisin(current.col, current.line, res);

    // verifier les 4 directions
    if(current.line-1 >= 0 && !Case_in_chemin(current.col, current.line-1, res) && l.m[current.col][current.line-1] != MUR && l.m[current.col][current.line-1] !=  VISITE) // left
        rec_find(l, res, (Case){current.col, current.line-1}, end);
    if(current.col - 1 >= 0 && !Case_in_chemin(current.col-1, current.line, res) && l.m[current.col-1][current.line] != MUR && l.m[current.col-1][current.line] !=  VISITE) // up
        rec_find(l, res, (Case){current.col-1, current.line}, end);   
    if(current.line+1 < l.cols && !Case_in_chemin(current.col, current.line+1, res) && l.m[current.col][current.line+1] != MUR && l.m[current.col][current.line+1] !=  VISITE) // down
        rec_find(l, res, (Case){current.col, current.line+1}, end);
    if(current.col+1 < l.lignes && !Case_in_chemin(current.col+1, current.line, res) && l.m[current.col+1][current.line] != MUR && l.m[current.col+1][current.line] !=  VISITE) // right
        rec_find(l, res, (Case){current.col+1, current.line}, end);
}

chemin solve_labyrinth(Laby l){
    // trouvons le depart
    Case start = (Case){UNUSED, UNUSED};
    for(int i=0; i< l.lignes; i++)
        for(int j=0; j< l.cols; j++)
            if(l.m[i][j] == ENTREE){
                start.col = i;
                start.line = j;
                //arret des boucles
                i = l.lignes;
                j = l.cols;
            }

    // trouvons l'arivee
    Case end = (Case){UNUSED, UNUSED};
    for(int i=0; i< l.lignes; i++)
        for(int j=0; j< l.cols; j++)
            if(l.m[i][j] == EXIT){
                end.col = i;
                end.line = j;
                //arret des boucles
                i = l.lignes;
                j = l.cols;
            }

    if(cases_egales(start, (Case){-1, -1}) || cases_egales(end, (Case){-1, -1})){
        printf("Impossible de trouver l'entree ou la sortie\n");
        exit(1);
    }

    chemin reponse = malloc(sizeof(chemin) * CHEMIN_LENGTH);
    
    for(int i = 0; i < CHEMIN_LENGTH; i++)
        reponse[i] = (Case){UNUSED, UNUSED};

    // lancer la recursivite
    rec_find(l, reponse, start, end);

    //remettre la case depart : 
    l.m[start.col][start.line] = ENTREE;

    // nettoyer la reponse si necessaire : 
    if(!check_solution(l, reponse)) nettoyer_chemin(reponse);

    return reponse;
}



void print_labyrinth(Laby l){
    //print upper indexs : 
    for(int i = 0 ; i < l.cols ; i++)
        printf("%d", i%10);
    printf("|+\n");
    for(int i=0; i < l.lignes; i++){
        for(int j=0; j < l.cols; j++)
            switch(l.m[i][j]){
                case MUR:
                    printf("#");
                    break;
                case WAY:
                    printf(" ");
                    break;
                case ENTREE:
                    printf("+");
                    break;
                case EXIT:
                    printf("-");
                    break;
                case VISITE:
                    printf(" ");
                    break;
                default:
                    printf("%c", l.m[i][j]);
                    break;
            }
        printf("|%d\n", i);
    }
}


int check_solution(Laby l, Case*Case_tab){
    //verifier ENTREE
    if(l.m[Case_tab[0].col][Case_tab[0].line] != ENTREE) return 0; //false

    int index_end;
    //avancer index_end jusqu'au cases inutilisees
    for(index_end = 0; index_end < CHEMIN_LENGTH-2 ; index_end++)
        if(Case_tab[index_end+1].col == UNUSED && Case_tab[index_end+1].line == UNUSED)
            break;

    if(index_end == CHEMIN_LENGTH - 1) return 0; // pas de fin de chemin

    //verifier que les cases sont voisines
    for(int i = 1; i <= index_end ; i++)
        if(!sont_voisines(Case_tab[i], Case_tab[i-1])) return 0;

    return 1; // tous tests passes
}

void free_labyrinth(Laby*l){
    if(l->m == NULL) return; // rien a faire

    for(int i = 0 ; i < l->lignes ; i++)
        if(l->m[i] != NULL) free(l->m[i]);

    if(l->m != NULL) free(l->m);

    l->m = NULL;
}

Laby creer_labyrinth(int cols, int lines){
    char* command = malloc(128);
    sprintf(command, "python3 generateur.py %d %d > out.txt",lines, cols);
    system(command);
    free(command);
    FILE* out = fopen("out.txt", "r");
    if (out == NULL){
        printf("Failed to open file.\n");
        exit(1);
    }
    
    Laby current_laby;
    current_laby.m = malloc(lines * sizeof(int *));
    for (int i = 0; i < lines; i++) current_laby.m[i] = (int *)malloc(cols * sizeof(int));
    
    int c;

    for (int i = 0; i < lines; i++) {
        for (int j = 0; j < cols; j++) 
            if ((c = fgetc(out)) != '\n') current_laby.m[i][j] = c - '0';
        fgetc(out); // ne pas prendre en compte le caracter "newline"
    }
    
    fclose(out);
    current_laby.lignes = lines;
    current_laby.cols = cols;
    return current_laby;
}

int abso(int x){return x < 0 ? -x : x;}
int sont_voisines(Case a, Case b){return (a.col == b.col && abso(a.line - b.line) == 1) || (a.line == b.line && abso(a.col - b.col) == 1);}
int cases_egales(Case a, Case b){return a.col == b.col && a.line == b.line;}
void print_Case(Case s){printf("%d %d\n", s.col, s.line);}