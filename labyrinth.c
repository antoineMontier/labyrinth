#include "labyrinth.h"  

chemin solve_labyrinth_threads(Laby l){
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

    // lancer la recursivite avec un thread !!!
    Thread_args args;
    args.l = &l;
    args.pere = pthread_self();
    args.res = malloc(NB_THREAD*sizeof(chemin));
    for(int i = 0 ; i < NB_THREAD ; ++i)
        args.res[i] = malloc(CHEMIN_LENGTH*sizeof(Case));
    for(int i = 0 ; i < NB_THREAD ; ++i)
        for(int j = 0 ; j < CHEMIN_LENGTH ; ++j)
            args.res[i][j] = (Case){UNUSED, UNUSED};

    args.assoc = malloc(NB_THREAD*sizeof(Thread_chemin));
    for(int i = 0 ; i < NB_THREAD ; ++i){
        args.assoc[i].id = 0; // pas de threads encore
        args.assoc[i].path = args.res[i];
    }
    args.assoc[0].id = args.pere; // enregistrer le pere
    args.end = &end;
    args.current = &start;
    args.tids = malloc(NB_THREAD*sizeof(pthread_t));
    args.fini = malloc(sizeof(int));
    *args.fini = 0;
    printf("avant le lancement de la recursivite\n");
    pthread_create(&(args.assoc[0].id), NULL, (void*)rec_find_thread,(void*)&args);
    printf("apres le lancement de la recursivite\n");

    for(int i = 0 ; i < NB_THREAD ; ++i)
        if(args.assoc[i].id != 0)
            pthread_join(args.assoc[i].id, NULL);
    printf("apres le join\n");


    // lire le chemin reponse dans le tableau de chemin : 

    chemin reponse_finale = malloc(CHEMIN_LENGTH * sizeof(Case));
    for(int i = 0 ; i < NB_THREAD ; ++i)
        if(args.res[i][0].col != UNUSED && args.res[i][0].line != UNUSED){ // only 1 
            for(int j = 0 ; j < CHEMIN_LENGTH ; ++j)
                reponse_finale[j] = args.res[i][j];
            i = NB_THREAD;
        }


            

    // rendre l'espace utilisé
    for(int i = 0 ; i < NB_THREAD ; ++i)
        free(args.res[i]);
    free(args.assoc);
    free(args.res);


    
    //remettre la case depart : 
    l.m[start.col][start.line] = ENTREE;

    // nettoyer la reponse si necessaire : 
    // if(!check_solution(l, reponse)) nettoyer_chemin(reponse);

    return reponse_finale;
}

int get_indice(Thread_args t){
    for(int i = 0 ; i < NB_THREAD ; ++i)
        if(t.assoc->id == pthread_self())
            return i;
    return -1;
}

int get_free_indice(Thread_args t) {
    for (int i = 0; i < NB_THREAD; ++i) {
        if (t.assoc[i].id == 0) {
            return i;
        }
    }
    printf("no available slot for new thread in 'int get_free_indice(Thread_args t)'\n");
    return -1;
}


int max_threads_reached(Thread_args t){
    for(int i = 0 ; i < NB_THREAD ; ++i)
        if(t.tids[i] == 0)
            return 0;
    return 1;
}


void* rec_find_thread(void* th_args){
    Thread_args* t = (Thread_args*)th_args;
    int thread_num = get_indice(*t);
    printf("I'm thread %ld\t located on slot c: %d | l: %d\tmax_threads_reached ? %d", pthread_self(), t->current->col, t->current->line, max_threads_reached(*t));
    fflush(stdout);

    if(*t->fini){ // chemin trouvé par un autre thread
        printf("je m'arrete car la solution est trouvee, fini = 1\n");
        // supprimer le chemin fait par le thread en marquant les cases comme inutilisees
        for(int i = 0 ; i < CHEMIN_LENGTH ; ++i)
            t->res[thread_num][0] = (Case){UNUSED, UNUSED};
        pthread_exit(NULL); // stopper le thread ici
    }

    if(cases_egales(*(t->current), *(t->end))){ // le thread actuel a trouvé la bonne réponse
        printf("je m'arrete car je suis sur la case reponse\n");

        *t->fini = 1; // communiquer aux autres threads qu'il faut qu'ils s'arretent
        ajouter_au_dernier_voisin(t->res[thread_num], *(t->current)); // ajouter la derniere case (end)
        pthread_exit(NULL); // stopper le thread ici
    }   


    // marquer la case comme visitée : 
    t->l->m[t->current->col][t->current->line] = VISITE;

    // ajouter la case dans le chemin du thread actuel
    ajouter_au_dernier_voisin(t->res[thread_num], *(t->current));

    // regarder dans les 4 directions

    int up = 0, left = 0, right = 0, down = 0, nb_direction = 0;
    if(t->current->line-1 >= 0 && !Case_in_chemin(t->current->col, t->current->line-1, t->res[thread_num]) && t->l->m[t->current->col][t->current->line-1] != MUR && t->l->m[t->current->col][t->current->line-1] !=  VISITE) up = 1;
    if(t->current->col - 1 >= 0 && !Case_in_chemin(t->current->col-1, t->current->line, t->res[thread_num]) && t->l->m[t->current->col-1][t->current->line] != MUR && t->l->m[t->current->col-1][t->current->line] !=  VISITE) left = 1;  
    if(t->current->line+1 < t->l->cols && !Case_in_chemin(t->current->col, t->current->line+1, t->res[thread_num]) && t->l->m[t->current->col][t->current->line+1] != MUR && t->l->m[t->current->col][t->current->line+1] !=  VISITE) down = 1;
    if(t->current->col+1 < t->l->lignes && !Case_in_chemin(t->current->col+1, t->current->line, t->res[thread_num]) && t->l->m[t->current->col+1][t->current->line] != MUR && t->l->m[t->current->col+1][t->current->line] !=  VISITE) right = 1;

    // ================ a supprimer 
    if(left)
        nb_direction++;
    if(up)
        nb_direction++;
    if(right)
        nb_direction++;
    if(left)
        nb_direction++;

    printf("\tI have %d directions possible\n", nb_direction);
    nb_direction = 0;


    // ================





    if(!left && !up && !right && !down){ // supprimer le chemin fait par le thread en marquant les cases comme inutilisees
        for(int i = 0 ; i < CHEMIN_LENGTH ; ++i)
            t->res[thread_num][0] = (Case){UNUSED, UNUSED};
        pthread_exit(NULL); // arreter le thread ici
    }


    if(up){
        printf("up nb_dir = %d\n", nb_direction); 
        Case new_current = (Case){t->current->col, t->current->line-1};
        Thread_args nt = (Thread_args){t->l, t->res, &new_current, t->end, t->tids, t->pere, t->assoc, t->fini};
        nb_direction++;
        if(nb_direction == 1 || max_threads_reached(*t)) // simple recusivité à lancer dans le thread actuel si une seule direction ou si le nb max de thread est atteint
            rec_find_thread((void*)&nt);
        else{ // lancer avec un thread
            printf("creating new thread.....\n");
            pthread_create((pthread_t*)(nt.assoc->id + get_free_indice(nt)), NULL, (void*)rec_find_thread, (void*)&nt);
        }
    }if(left){
        printf("left nb_dir = %d\n", nb_direction);   
        Case new_current = (Case){t->current->col-1, t->current->line};
        Thread_args nt = (Thread_args){t->l, t->res, &new_current, t->end, t->tids, t->pere, t->assoc, t->fini};
        nb_direction++;
        if(nb_direction == 1 || max_threads_reached(*t)) // simple recusivité à lancer dans le thread actuel si une seule direction ou si le nb max de thread est atteint
            rec_find_thread((void*)&nt);
        else{ // lancer avec un thread
            printf("creating new thread.....\n");
            pthread_create((pthread_t*)(nt.assoc->id + get_free_indice(nt)), NULL, (void*)rec_find_thread, (void*)&nt);
        }
    }if(down){
        printf("down nb_dir = %d\n", nb_direction);   
        Case new_current = (Case){t->current->col, t->current->line+1};
        Thread_args nt = (Thread_args){t->l, t->res, &new_current, t->end, t->tids, t->pere, t->assoc, t->fini};
        nb_direction++;
        if(nb_direction == 1 || max_threads_reached(*t)) // simple recusivité à lancer dans le thread actuel si une seule direction ou si le nb max de thread est atteint
            rec_find_thread((void*)&nt);
        else{ // lancer avec un thread
            printf("creating new thread.....\n");
            pthread_create((pthread_t*)(nt.assoc->id + get_free_indice(nt)), NULL, (void*)rec_find_thread, (void*)&nt);
        }
    }if(right){
        printf("right nb_dir = %d\n", nb_direction);  
        Case new_current = (Case){t->current->col+1, t->current->line};
        Thread_args nt = (Thread_args){t->l, t->res, &new_current, t->end, t->tids, t->pere, t->assoc, t->fini};
        nb_direction++;
        if(nb_direction == 1 || max_threads_reached(*t)) // simple recusivité à lancer dans le thread actuel si une seule direction ou si le nb max de thread est atteint
            rec_find_thread((void*)&nt);
        else{ // lancer avec un thread
            printf("creating new thread.....\n");
            pthread_create((pthread_t*)(nt.assoc->id + get_free_indice(nt)), NULL, (void*)rec_find_thread, (void*)&nt);
        }
    }

    return NULL;
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