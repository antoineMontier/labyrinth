#include "labyrinth.h"  

Thread_args * global_args;


void print_ids(){
    pthread_mutex_lock(&acces_out);                     //aussi lock l'acces memoire ?
    printf("\n\n=================Threads _ids :\n");
    for(int i =  0; i < NB_THREAD ; ++i){
        printf("%ld -- ", global_args->threads[i]);
        print_chemin(global_args->res[i]);
    }
    printf("=================\n\n");
    pthread_mutex_unlock(&acces_out);
}

int getLastCaseIndex(int thread_index){
    if(thread_index == -1){
        print("getLastCaseIndex error, thread index is -1");
        exit(1);
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
    global_args->fini = malloc(1*sizeof(int));
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

    pthread_mutex_unlock(&acces_out);
    pthread_mutex_unlock(&acces_ids);
    pthread_mutex_unlock(&acces_laby);

    printf("avant le lancement de la recursivite\n");
    pthread_create(&(global_args->threads[0]), NULL, (void*)rec_find_thread,NULL);
    //while(*(global_args->fini) == 0)print_sols();
    printf("apres le lancement de la recursivite\n");

    for(int i = 0 ; i < NB_THREAD ; ++i)
        if(!(*(global_args->fini)) && global_args->threads[i] != 0){
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
        }else if(i == NB_THREAD)
            printf("pas de chemin retenu :(\n");
    }
            

    // rendre l'espace utilisé
    for(int i = 0 ; i < NB_THREAD ; ++i)
        free(global_args->res[i]);
    free(global_args->threads);
    free(global_args->res);
    free(global_args->fini);
    free(global_args);


    
    //remettre la case depart : 
    l.m[start.col][start.line] = ENTREE;
    //et celle d'arrivee
    l.m[end.col][end.line] = EXIT;

    // nettoyer la reponse si necessaire : 
    // if(!check_solution(l, reponse)) nettoyer_chemin(reponse);

    return reponse_finale;
}

/// @brief avoir l'acces_ids
/// @return l'indice correspondant au thread.
int get_thread_num(){
    for(int i = 0 ; i < NB_THREAD ; ++i)
        if(global_args->threads[i] == pthread_self()){
            return i;
        }
    return -1;
}

/// @brief avoir l'acces_ids
/// @return un indice dispo
int get_first_room_for_new_thread(){
    for(int i = 0 ; i < NB_THREAD ; ++i)
        if(global_args->threads[i] == 0)
            return i;
    return -1;
}

/// @brief avoir l'acces_ids
/// @param thread_index indice du thread
void stopper_thread_et_reset_chemin(int thread_index){
    if(thread_index == -1){
        print("erreure dans stopper_thread_et_reset_chemin ; thread index = -1");
        exit(1);
    }

    for(int i = 0 ; i < CHEMIN_LENGTH ; ++i)
        global_args->res[thread_index][i] = (Case){UNUSED, UNUSED};
    global_args->threads[thread_index] = 0;
    pthread_exit(NULL);
}

void print(const char * msg){
    pthread_mutex_lock(&acces_out);
    printf("%ld:\t%s\n", pthread_self(), msg);
    pthread_mutex_unlock(&acces_out);
}

void marquer_la_case_visitee(int col, int line){
    if(col < 0 || line < 0){
        print("erreur indice, function marquer_la_case_visitee");
        exit(1);
    }
    pthread_mutex_lock(&acces_ids); // ======= lock
    global_args->l->m[col][line] = VISITE;
    pthread_mutex_unlock(&acces_ids); // ===== unlock
}

/// @brief utiliser avec acces_ids -- optimisation possible
/// @param from 
/// @param to 
void copier_chemins(int from, int to){
    if(from < 0 || from >= NB_THREAD || to < 0 || to >= NB_THREAD){
        print("Erreur : Mauvais indice de chemins dans la fonction copier_chemins");
        exit(1);
    }if(to == from)
        return;
    for(int i = 0 ; i < CHEMIN_LENGTH ; i++)                    // --- a optimiser avec {-1 , -1}
        global_args->res[to][i] = global_args->res[from][i];
}

void rec_find_thread(){

    // ================ ARRET : FINI = TRUE =================
    if(*(global_args->fini)) pthread_exit(NULL);
    // vérifier si le thread actuel est sur la case réponse

    // ================ ARRET : CASE END ====================
    pthread_mutex_lock(&acces_ids); // ======= lock
    int thread_index = get_thread_num();
    int case_index = getLastCaseIndex(thread_index);
    if(case_index >= CHEMIN_LENGTH - 2){
        print("pas assez de memoire allouee au vecteur de chemins");
        exit(1);
    }
    int ln = global_args->res[thread_index][case_index].line, cl = global_args->res[thread_index][case_index].col;
    pthread_mutex_unlock(&acces_ids); // ===== unlock

    if(cases_egales((Case){cl, ln}, global_args->end)){
        *global_args->fini = 1;
        pthread_exit(NULL);
    }



                            // ========== CASE VISITEE =================================
                            // marquer_la_case_visitee(cl, ln); // mutexs geres dans la fonction

    // ============== CHECK DIRECTIONS =========================
    int directions_explorees = 0;
    if(ln-1 >= 0){ // ========================================== UP -- cette direction est a optimiser (lancer directement en recursivité, ne pas verifier directions_explorees)
        pthread_mutex_lock(&acces_laby);    
        if(global_args->l->m[cl][ln-1] != MUR  && global_args->l->m[cl][ln-1] !=  VISITE){
            global_args->l->m[cl][ln-1] = VISITE; // marquer la case comme visitee
            pthread_mutex_unlock(&acces_laby);

            pthread_mutex_lock(&acces_ids);
            if(!Case_in_chemin(cl, ln-1, global_args->res[thread_index])){
            

                int indice_libre = get_first_room_for_new_thread();
                if(directions_explorees == 0 || indice_libre == -1){
                    // ==================== recursivite simple
                    global_args->res[thread_index][case_index + 1] = (Case){cl, ln-1}; // ajouter la case suivante au vecteur
                    pthread_mutex_unlock(&acces_ids); // deverouiller l'acces memoire des que possible
                    rec_find_thread(); // lancer la recursivite
                }else{
                    // ==================== thread 
                    copier_chemins(thread_index, indice_libre);
                    pthread_create(&(global_args->threads[indice_libre]), NULL, (void*)rec_find_thread, NULL);
                    pthread_mutex_unlock(&acces_ids);
                }

                directions_explorees++;
            }else pthread_mutex_unlock(&acces_ids);
        }else pthread_mutex_unlock(&acces_laby);
    }




    if(cl-1 >= 0){ // ========================================== LEFT
        pthread_mutex_lock(&acces_laby);    
        if(global_args->l->m[cl-1][ln] != MUR  && global_args->l->m[cl-1][ln] !=  VISITE){
            global_args->l->m[cl-1][ln] = VISITE; // marquer la case comme visitee
            pthread_mutex_unlock(&acces_laby);

            pthread_mutex_lock(&acces_ids);
            if(!Case_in_chemin(cl-1, ln, global_args->res[thread_index])){
            

                int indice_libre = get_first_room_for_new_thread();
                if(directions_explorees == 0 || indice_libre == -1){
                    // ==================== recursivite simple
                    global_args->res[thread_index][case_index + 1] = (Case){cl-1, ln}; // ajouter la case suivante au vecteur
                    pthread_mutex_unlock(&acces_ids); // deverouiller l'acces memoire des que possible
                    rec_find_thread(); // lancer la recursivite
                }else{
                    // ==================== thread 
                    copier_chemins(thread_index, indice_libre);
                    pthread_create(&(global_args->threads[indice_libre]), NULL, (void*)rec_find_thread, NULL);
                    pthread_mutex_unlock(&acces_ids);
                }

                directions_explorees++;
            }else pthread_mutex_unlock(&acces_ids);
        }else pthread_mutex_unlock(&acces_laby);
    }




    if(ln+1 < /*MAX_LINES*/ 0){ // ========================================== DOWN
        pthread_mutex_lock(&acces_laby);    
        if(global_args->l->m[cl][ln+1] != MUR  && global_args->l->m[cl][ln+1] !=  VISITE){
            global_args->l->m[cl][ln+1] = VISITE; // marquer la case comme visitee
            pthread_mutex_unlock(&acces_laby);

            pthread_mutex_lock(&acces_ids);
            if(!Case_in_chemin(cl, ln+1, global_args->res[thread_index])){
            

                int indice_libre = get_first_room_for_new_thread();
                if(directions_explorees == 0 || indice_libre == -1){
                    // ==================== recursivite simple
                    global_args->res[thread_index][case_index + 1] = (Case){cl, ln+1}; // ajouter la case suivante au vecteur
                    pthread_mutex_unlock(&acces_ids); // deverouiller l'acces memoire des que possible
                    rec_find_thread(); // lancer la recursivite
                }else{
                    // ==================== thread 
                    copier_chemins(thread_index, indice_libre);
                    pthread_create(&(global_args->threads[indice_libre]), NULL, (void*)rec_find_thread, NULL);
                    pthread_mutex_unlock(&acces_ids);
                }

                directions_explorees++;
            }else pthread_mutex_unlock(&acces_ids);
        }else pthread_mutex_unlock(&acces_laby);
    }





    if(cl+1 < /*MAX_COLS*/ 0){ // ========================================== DOWN
        pthread_mutex_lock(&acces_laby);    
        if(global_args->l->m[cl+1][ln] != MUR  && global_args->l->m[cl+1][ln] !=  VISITE){
            global_args->l->m[cl+1][ln] = VISITE; // marquer la case comme visitee
            pthread_mutex_unlock(&acces_laby);

            pthread_mutex_lock(&acces_ids);
            if(!Case_in_chemin(cl+1, ln, global_args->res[thread_index])){
            

                int indice_libre = get_first_room_for_new_thread();
                if(directions_explorees == 0 || indice_libre == -1){
                    // ==================== recursivite simple
                    global_args->res[thread_index][case_index + 1] = (Case){cl+1, ln}; // ajouter la case suivante au vecteur
                    pthread_mutex_unlock(&acces_ids); // deverouiller l'acces memoire des que possible
                    rec_find_thread(); // lancer la recursivite
                }else{
                    // ==================== thread 
                    copier_chemins(thread_index, indice_libre);
                    pthread_create(&(global_args->threads[indice_libre]), NULL, (void*)rec_find_thread, NULL);
                    pthread_mutex_unlock(&acces_ids);
                }

                directions_explorees++;
            }else pthread_mutex_unlock(&acces_ids);
        }else pthread_mutex_unlock(&acces_laby);
    }


    // Apres les 4 ifs de fonctions recursives : 
    print("fin de fonction");
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