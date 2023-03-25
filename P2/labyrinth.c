#include "labyrinth.h"  

Thread_args * global_args;
pthread_mutex_t acces_ids = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t acces_laby = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t acces_out = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t acces_ids_history = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t solution_trouvee = PTHREAD_MUTEX_INITIALIZER;




void print_ids(){
    pthread_mutex_lock(&acces_out);                     //aussi lock l'acces memoire ?
    printf("\n\n=================Threads _ids :\n");
    for(int i =  0; i < NB_THREAD ; ++i){
        printf("%p -- ", (void*)global_args->threads[i]);
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

void ajouter_dans_historique(pthread_t thread_id){
    pthread_mutex_lock(&acces_ids_history);
    for(int i = 0 ; i < NB_THREAD_TOATL ; ++i){
        if(global_args->threads_history[i] == thread_id){
            pthread_mutex_unlock(&acces_ids_history);
            return; // deja present
        }if(global_args->threads_history[i] == 0){
            global_args->threads_history[i] = thread_id;
            pthread_mutex_unlock(&acces_ids_history);
            return;
        }
        if(i == NB_THREAD_TOATL){
            print("Erreur: nombre maximal de thread dans l'historique atteint, augmenter le #define NB_THREAD_TOATL");
            exit(1);
        }
    }
}

chemin solve_labyrinth_threads(Laby l){ 
    // si NB_THREAD == 1, lancer en recursif normal
    if(NB_THREAD <= 1) return solve_labyrinth(l);

    Case start = trouver_entree(l);
    Case end = trouver_sortie(l);  
    l.m[start.col][start.line] = VISITE;

    // allouer la struct partagee par les threads
    global_args = malloc(sizeof(Thread_args));
    // == donner l'acces en lecture/ecriture au labyrinth
    global_args->l = &l;
    // == donner l'acces en lecture a la case sortie
    global_args->end = end;
    // == allouer le vecteur de chemins
    global_args->res = malloc(NB_THREAD*sizeof(chemin));
    for(int i = 0 ; i < NB_THREAD ; i++){
        // == allouer chaque chemin du vecteur
        global_args->res[i] = malloc(CHEMIN_LENGTH*sizeof(chemin));
        // == initialiser les chemins
        for(int j = 1 ; j < CHEMIN_LENGTH ; j++)
            global_args->res[i][j] = (Case){UNUSED, UNUSED};
        // == ajouter la case depart a chaque vecteur de chemins
        global_args->res[i][0] = (Case){start.col, start.line};
    }
    // == allouer le stockage des threads (threads utilises simultanement)
    global_args->threads = malloc(NB_THREAD*sizeof(pthread_t));
    // == initialiser pour ne pas avoir de mauvaise surprise
    for(int i = 0 ; i < NB_THREAD ; i++)
        global_args->threads[i] = 0;
    // == allouer le stockage de l'historique des threads (tous les threads utilises) ; sert a s'assurer lors du join que tout le monde a bien fini
    global_args->threads_history = malloc(NB_THREAD_TOATL * sizeof(pthread_t));
    // == initialiser pour ne pas avoir de mauvaise surprise
    for(int i = 0 ; i < NB_THREAD_TOATL ; i++)
        global_args->threads_history[i] = 0;
    // == allouer le chemin final
    chemin reponse_finale = malloc(CHEMIN_LENGTH * sizeof(Case));
    // == initialiser
    for(int i = 0; i < CHEMIN_LENGTH; i++)
        reponse_finale[i] = (Case){UNUSED, UNUSED};

    // == mettre le mutex de solution trouvee a lock (pas de solution encore trouvee)
    pthread_mutex_lock(&solution_trouvee);


    pthread_create(&(global_args->threads[0]), NULL, (void*)rec_find_thread, NULL); // lancer la fonction dans un thread
    // directement enregister le thread dans lequel la fonction a ete lancee dans l'historique
    ajouter_dans_historique(global_args->threads[0]);


    pthread_mutex_lock(&solution_trouvee); // faire attendre le thread principale

    // join les threads (les chemins parcourus restent en memoire)
    for(int i = 0 ; i < NB_THREAD_TOATL ; ++i)
        if(global_args->threads_history[i] > 0){
            pthread_join(global_args->threads_history[i], NULL);
            global_args->threads_history[i] = -1;
        }

    

    // lire le chemin reponse dans le tableau de chemin : 

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
            printf("Erreur: pas de chemin retenu\n");
    }

    // rendre l'espace utilisé
    for(int i = 0 ; i < NB_THREAD ; ++i)
        free(global_args->res[i]);
    free(global_args->res);
    free(global_args->threads);
    free(global_args->threads_history);
    free(global_args);


    
    // remettre les cases depart & arrivee (peuvent etre malencontreusement remplacees par des cases VISITE lors de la recursivite)
    l.m[start.col][start.line] = ENTREE; l.m[end.col][end.line] = EXIT;

    // a voir si necessaire
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


void print(const char * msg){
    pthread_mutex_lock(&acces_out);
    printf("%p:\t%s\n", (void*)pthread_self(), msg);
    pthread_mutex_unlock(&acces_out);
}

/*void marquer_la_case_visitee(int col, int line){
    if(col < 0 || line < 0){
        print("erreur indice, function marquer_la_case_visitee");
        exit(1);
    }
    pthread_mutex_lock(&acces_ids); // ======= lock
    global_args->l->m[col][line] = VISITE;
    pthread_mutex_unlock(&acces_ids); // ===== unlock
}*/

/// @brief utiliser avec acces_ids
/// @param from 
/// @param to 
void copier_chemins(int from, int to){
    if(from < 0 || from >= NB_THREAD || to < 0 || to >= NB_THREAD){
        print("Erreur : Mauvais indice de chemins dans la fonction copier_chemins");
        exit(1);
    }if(to == from)
        return;
    for(int i = 0 ; i < CHEMIN_LENGTH; i++)
        if(cases_egales(global_args->res[to][i] = global_args->res[from][i], (Case){-1, -1}))
            break;
}

int nombre_ways(int col, int line){ // -- pas sur que cette fonction est vraiment utilisee
    int count = 0;
    pthread_mutex_lock(&acces_laby);
    if(line-1 >= 0 && (global_args->l->m[col][line-1] != MUR && global_args->l->m[col][line-1] != VISITE))  ++count;
    if(col-1 >= 0 && (global_args->l->m[col-1][line] != MUR && global_args->l->m[col-1][line] != VISITE))  ++count;
    if(line+1 < global_args->l->lignes && (global_args->l->m[col][line+1] != MUR && global_args->l->m[col][line+1] != VISITE))  ++count;
    if(col+1 < global_args->l->cols && (global_args->l->m[col+1][line] != MUR && global_args->l->m[col+1][line] != VISITE))  ++count;
    pthread_mutex_unlock(&acces_laby);  
    //printf("{%d ; %d} : %d directions\t\t", col, line, count);
    //print("");
    return count;
}


int possibilites_de_mouvement(Case c){ // -- optimiser avec une variable temporaire // -- <= ou < ??? si oui <
    if(cases_egales(c, (Case){UNUSED, UNUSED}))
        return 0;
    int count = 0;
    if(c.line-1 >= 0){
        printf("\t\tUP\tc.line-1 >= 0\tcount = %d\t\tm[%d][%d] = %d\n", count,c.col, c.line-1, global_args->l->m[c.col][c.line-1]);
        if(global_args->l->m[c.col][c.line-1] != MUR && global_args->l->m[c.col][c.line-1] != VISITE){++count;printf("case {%d, %d} a une possibilite en haut\n", c.col, c.line);} 
    }
    if(c.col-1 >= 0){
        printf("\t\tLEFT\tc.col-1 >= 0\tcount = %d\t\tm[%d][%d] = %d\n", count,c.col-1, c.line, global_args->l->m[c.col-1][c.line]);
        if(global_args->l->m[c.col-1][c.line] != MUR && global_args->l->m[c.col-1][c.line] != VISITE){++count;printf("case {%d, %d} a une possibilite en gauche\n", c.col, c.line);} 
    }
    if(c.line+1 < global_args->l->lignes){
        printf("\t\tDOWN\tc.line+1 < %d\tcount = %d\t\tm[%d][%d] = %d\n", global_args->l->lignes, count,c.col, c.line+1, global_args->l->m[c.col][c.line+1]);
        if(global_args->l->m[c.col][c.line+1] != MUR && global_args->l->m[c.col][c.line+1] != VISITE){++count;printf("case {%d, %d} a une possibilite en bas\n", c.col, c.line);} 
    }
    if(c.col+1 < global_args->l->cols){
        printf("\t\tRIGHT\tc.col+1 < %d\tcount = %d\t\tm[%d][%d] = %d\n", global_args->l->cols, count, c.col+1, c.line, global_args->l->m[c.col+1][c.line]);
        if(global_args->l->m[c.col+1][c.line] != MUR && global_args->l->m[c.col+1][c.line] != VISITE){++count;printf("case {%d, %d} a une possibilite en droite\n", c.col, c.line);} 
    }
    return count;
}

int est_dans_un_cul_de_sac(int t_id){
    if(t_id == -1){
        print("erreur, identifiant de thread == -1 dans la fonction est_dans_un_cul_de_sac(int)");
        exit(1);
    }
    /*
    for(int i = 0 ;  i < CHEMIN_LENGTH && !cases_egales(global_args->res[t_id][i], (Case){UNUSED, UNUSED}) ; i++){
        printf("i = %d : {%d, %d}\n", i, global_args->res[t_id][i].col, global_args->res[t_id][i].line);
        if(possibilites_de_mouvement(global_args->res[t_id][i]) != 0)
            return 0; // pas de cul de sac
    }
    */

    for(int i = 0 ;  i < CHEMIN_LENGTH ; i++){
        printf("i = %d : {%d, %d}\n", i, global_args->res[t_id][i].col, global_args->res[t_id][i].line);
        if(possibilites_de_mouvement(global_args->res[t_id][i]) != 0){
            printf("case {%d, %d} a une posibilite de mouvement\n", global_args->res[t_id][i].col, global_args->res[t_id][i].line);
            return 0; // pas de cul de sac
        }
        if(cases_egales(global_args->res[t_id][i], (Case){UNUSED, UNUSED}) ){
            printf("case {%d, %d} == {UNUSED, UNUSED}\n", global_args->res[t_id][i].col, global_args->res[t_id][i].line);
            return 1;
        }
    }
    return 1;
}



void rec_find_thread(){
    
    print_ids(); // affiche les thread_t ainsi que le chemin qu'ils ont parcouru
    // printf("aaaa\n");
    // ================ ARRET : solution trouvee =================
    if(pthread_mutex_trylock(&solution_trouvee) == 0) {pthread_mutex_unlock(&solution_trouvee); pthread_exit(NULL);}
    // vérifier si le thread actuel est sur la case réponse

    // ================ Recuperer le n° de thread, la case sur laquelle il se trouve et lrd caracteristiques de cette cas =================
    pthread_mutex_lock(&acces_ids); // ======= lock

    int thread_index = get_thread_num(); // n° de thread
    int case_index = getLastCaseIndex(thread_index); // n° de case
    if(case_index >= CHEMIN_LENGTH - 2){
        print("pas assez de memoire allouee au vecteur de chemins");
        exit(1);
    }
    int ln = global_args->res[thread_index][case_index].line, cl = global_args->res[thread_index][case_index].col; // n° de ligne et de colonne

    pthread_mutex_unlock(&acces_ids); // ===== unlock
    // printf("bbbbb\n");

    // ================ ARRET : je suis sur la case end ====================
    if(cases_egales((Case){cl, ln}, global_args->end)){
        pthread_mutex_lock(&acces_ids); // lock ici pour attendre que tout le mode ecrive dans sa memoire associee et eviter les segfault
        pthread_mutex_unlock(&acces_ids);
        pthread_mutex_unlock(&solution_trouvee); // debloquer pour signaler que la solution est trouvee
        pthread_exit(NULL);
    }
    // ============== CHECK DIRECTIONS ========================= Je nai commente que pour la direction UP, les 4 directions ont le meme code, elles different par les cases dans laquelle la recurtisvite / thread est lance.
    if(ln-1 >= 0){ // ========================================== UP

        pthread_mutex_lock(&acces_laby);       // lock le labyrinth pour ne pas se melanger avec les autres threads (exemple d'un thread qui voudrait visiter la même case en meme temps, les deux voient qulle est libre... et cest le bazar)
        if(global_args->l->m[cl][ln-1] ==  WAY || global_args->l->m[cl][ln-1] ==  EXIT){ // verifier que c'est possible d'xplorer la case ( condition equivalente a [global_args->l->m[cl][ln-1] !=  MUR && global_args->l->m[cl][ln-1] !=  VISITE])
            global_args->l->m[cl][ln-1] = VISITE; // marquer la case comme visitee
            pthread_mutex_unlock(&acces_laby); // liberer lacces au labyrinth

            pthread_mutex_lock(&acces_ids); // bloquer l'acces a la memoire des identifiants de threads ainsi que leur memoires correspondant aux chemins
            if(!Case_in_chemin(cl, ln-1, global_args->res[thread_index])){ // verifier que la case a ajouter n'est pas deja dans le chemin --optimisation ?
                int indice_libre = get_first_room_for_new_thread(); // retourne un indice libre. si pas d'indice libre, retourne -1
                if(nombre_ways(cl, ln) == 0 ||  indice_libre == -1){ /*indice_libre == -1 signifie que le nombre max de thread est atteint // nombre_ways(cl, ln) == 0 est vrai si a partir de la case actuelle, il y a seulement une direction possible (on regarde par rapport a 0 car la direction actuelle ne sera pas comptee par la fonction car elle a ete marquee comme VISITEE un peu plus haut)*/
                    //print(">recursivite UP");// ==================== recursivite simple
                    // print("ajout UP");
                    ajouter_coord_et_nettoyer_apres(cl, ln-1, global_args->res[thread_index]); // nettoie toutes les cases apres celle que l'on vient d'ajouter. la case que l'on vient d'ajouter a ete ajoutee a cote d'une case voisine (selon la logique du backtrack)
                    pthread_mutex_unlock(&acces_ids); // deverouiller l'acces memoire des que possible
                    rec_find_thread(); // lancer la recursivite
                }else{
                    //print(">thread");// ==================== thread 
                    copier_chemins(thread_index, indice_libre); // copier le chemin parcouru pour que le nv thread sache où il est et en cas de solution, il puisse donner le chemin entier
                    global_args->res[indice_libre][case_index + 1] = (Case){cl, ln-1}; // ajouter la case suivante au vecteur
                    pthread_create(&(global_args->threads[indice_libre]), NULL, (void*)rec_find_thread, NULL); // lancer le thread, grace aux varaibles partagees, pas besoin d'arguments ni de valeurs de retour
                    pthread_mutex_unlock(&acces_ids); // apres (pas avant car le lancement de thread ecrit dans la memoire la valeur du pthread_t cree) on libere l'acces memoire.
                    ajouter_dans_historique(global_args->threads[indice_libre]); // on ajoute dans l'historique juste apres.
                }
            }else pthread_mutex_unlock(&acces_ids); // debloquer l'acces memoire si on n'est pas rentre dans le if
        }else pthread_mutex_unlock(&acces_laby); // liberer l'acces au labyrinth si on n'est pas rentre dans le if (pas de chemin possible...)
    }
    // printf("cccc\n");

    if(cl-1 >= 0){ // ========================================== LEFT
        pthread_mutex_lock(&acces_laby);    
        if(global_args->l->m[cl-1][ln] ==  WAY || global_args->l->m[cl-1][ln] ==  EXIT){
            global_args->l->m[cl-1][ln] = VISITE; // marquer la case comme visitee
            pthread_mutex_unlock(&acces_laby);

            pthread_mutex_lock(&acces_ids);
            if(!Case_in_chemin(cl-1, ln, global_args->res[thread_index])){
                int indice_libre = get_first_room_for_new_thread();
                if(nombre_ways(cl, ln) == 0 ||  indice_libre == -1){
                    // print("ajout LEFT");
                    ajouter_coord_et_nettoyer_apres(cl-1, ln, global_args->res[thread_index]);
                    pthread_mutex_unlock(&acces_ids);
                    rec_find_thread();
                }else{
                    copier_chemins(thread_index, indice_libre);
                    global_args->res[indice_libre][case_index + 1] = (Case){cl-1, ln};
                    pthread_create(&(global_args->threads[indice_libre]), NULL, (void*)rec_find_thread, NULL);
                    pthread_mutex_unlock(&acces_ids);
                    ajouter_dans_historique(global_args->threads[indice_libre]);
                }
            }else pthread_mutex_unlock(&acces_ids);
        }else pthread_mutex_unlock(&acces_laby);
    }
    // printf("dddd\n");

    if(ln+1 < global_args->l->lignes){ // ========================================== DOWN
        pthread_mutex_lock(&acces_laby);    
        if(global_args->l->m[cl][ln+1] ==  WAY || global_args->l->m[cl][ln+1] ==  EXIT){
            global_args->l->m[cl][ln+1] = VISITE;
            pthread_mutex_unlock(&acces_laby);

            pthread_mutex_lock(&acces_ids);
            if(!Case_in_chemin(cl, ln+1, global_args->res[thread_index])){
            

                int indice_libre = get_first_room_for_new_thread();
                if(nombre_ways(cl, ln) == 0 ||  indice_libre == -1){
                    // print("ajout DOWN");
                    ajouter_coord_et_nettoyer_apres(cl, ln+1, global_args->res[thread_index]);
                    pthread_mutex_unlock(&acces_ids);
                    rec_find_thread();
                }else{
                    copier_chemins(thread_index, indice_libre);
                    global_args->res[indice_libre][case_index + 1] = (Case){cl, ln+1};
                    pthread_create(&(global_args->threads[indice_libre]), NULL, (void*)rec_find_thread, NULL);
                    pthread_mutex_unlock(&acces_ids);
                    ajouter_dans_historique(global_args->threads[indice_libre]);
                }
            }else pthread_mutex_unlock(&acces_ids);
        }else pthread_mutex_unlock(&acces_laby);
    }

    // printf("eeeee\n");

    if(cl+1 < global_args->l->cols){ // ========================================== RIGHT
        pthread_mutex_lock(&acces_laby);    
        if(global_args->l->m[cl+1][ln] ==  WAY || global_args->l->m[cl+1][ln] ==  EXIT){
            global_args->l->m[cl+1][ln] = VISITE;
            pthread_mutex_unlock(&acces_laby);
            pthread_mutex_lock(&acces_ids);
            if(!Case_in_chemin(cl+1, ln, global_args->res[thread_index])){
                int indice_libre = get_first_room_for_new_thread();
                if(nombre_ways(cl, ln) == 0 ||  indice_libre == -1){
                    // print("ajout RIGHT");
                    ajouter_coord_et_nettoyer_apres(cl+1, ln, global_args->res[thread_index]);
                    pthread_mutex_unlock(&acces_ids);
                    rec_find_thread();
                }else{
                    copier_chemins(thread_index, indice_libre);
                    global_args->res[indice_libre][case_index + 1] = (Case){cl+1, ln};
                    pthread_create(&(global_args->threads[indice_libre]), NULL, (void*)rec_find_thread, NULL);
                    pthread_mutex_unlock(&acces_ids);
                    ajouter_dans_historique(global_args->threads[indice_libre]);
                }
            }else pthread_mutex_unlock(&acces_ids);
        }else pthread_mutex_unlock(&acces_laby);
    }

    // Apres les 4 ifs de fonctions recursives :    
    /* 
    si le code en dessous est décommenté, dans chaque thread, des qu'une recursivite se trouvera dans un cul de sac, elle commandera l'arret de son propre thread.
    Cela empeche tout backtrack mais permet de recycler les threads : un thread est fini, on le communique et l'espace est dispo des que necessaire.
    Il faudrait donc un moyen efficace pour savoir si un thread est dans un cul de sac et qu'il ne peut plus creer de backtracks (encerclé par des cases VISITE || MUR).

    A l'origine, directions_explorees correspond au nombre de directions dans lesquels une recursivite ou un thread a ete lance (if dans lequels la fonction est rentree, entre 0 et 3) 
    */

    /*if(directions_explorees == 0){
        print("suppression du thread"); 
        pthread_mutex_lock(&acces_ids);
        for(int i = 0 ; i <= case_index + 1  ; ++i )
            global_args->res[thread_index][i] = (Case){UNUSED, UNUSED};
        global_args->threads[thread_index] = 0;
        pthread_mutex_unlock(&acces_ids);
        print("supprime, maintenant exit");
        pthread_exit(NULL);
    }*/
    // printf("ffff\n");


    if(est_dans_un_cul_de_sac(thread_index)){
        // printf("gggg\n");


        //printf("iiii\n");
        print("exit");
        pthread_mutex_lock(&acces_out);
        printf("est dans un cul de sac : %d\n", est_dans_un_cul_de_sac(thread_index));
        print_raw_labyrinth(*global_args->l);
        print_labyrinth(*global_args->l);
        pthread_mutex_lock(&acces_ids);
        // printf("hhhhh\n");

        for(int i = 1 ; i <= case_index + 1  ; ++i )
            global_args->res[thread_index][i] = (Case){UNUSED, UNUSED};
        global_args->threads[thread_index] = 0;
        pthread_mutex_unlock(&acces_ids);        pthread_mutex_unlock(&acces_out);
        pthread_exit(NULL);
    }
    // printf("jjjjj\n");

}


void print_solution(Laby l, chemin c){
    // ajouter des lettres dans le labyrinth pour montrer le chemin pris
    int ch = 'a';
    for(int i=1; i<CHEMIN_LENGTH-1 ; ++i){
        if(c[i+1].col == UNUSED && c[i+1].line == UNUSED)
            break;
        else
            l.m[c[i].col][c[i].line] = ch;//'@'; // affichage du chemin avec les characteres ascii a,b->z,A,B->Z
        ++ch;
        if(ch == 'z' + 1)
            ch = 'A';
        if(ch == 'Z' + 1)
            ch = 'a';
        }

    print_labyrinth(l);

    // retirer les lettres
    for(int i=1; i < CHEMIN_LENGTH - 1 ; ++i){
        if(c[i].col == UNUSED && c[i].line ==UNUSED)
            break;
        else
            l.m[c[i].col][c[i].line] = WAY;
        }
}


void print_chemin(chemin c){
    printf("[");
    for(int i = 0; i < CHEMIN_LENGTH ; i++){
        if(cases_egales(c[i], (Case){UNUSED, UNUSED}))
            break;
        printf(" %d;%d ", c[i].col, c[i].line);
    }
    printf("]\n");
}


void ajouter_coordonees_au_chemin_au_dernier_voisin(int col, int line, chemin c){// -- optimisation
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

int ajouter_coord_et_nettoyer_apres(int col, int line, chemin c){
    if(col == -1 || line == -1){
        print("erreur dans le format de la case");
        return 0;
    }
    Case s = {col, line};
    for(int i = CHEMIN_LENGTH -2 ; i >= 0 ; --i){
        if(c[i].col != UNUSED && c[i].line != UNUSED){
            if(sont_voisines(c[i], s) || cases_egales(c[i], s)){// skipper toutes les cases à la fin de coordonnees{-1 ; -1} --optimisation du if possible
                c[i+1] = s; // case ajoutee
                for(int j = i + 2 ; j < CHEMIN_LENGTH ; ++j)
                    c[j] = (Case){UNUSED, UNUSED};
                return 1;
            }
        }
    }
    printf("echec d'ajout de la case {%d, %d}", col, line); print("");
    print_ids();
    exit(1);
    return 0;// aucune case ajoutee
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
    for(int i=0; i < CHEMIN_LENGTH; i++) // --optimisation
        if(c[i].col == col && c[i].line == line)
            return 1;
    return 0;
}


void print_raw_labyrinth(Laby l){
    //print upper indexs : 
    for(int i = 0 ; i < l.cols ; i++)
        printf("%d\t", i%10);
    printf("|+\n");
    for(int i=0; i < l.lignes; i++){
        for(int j=0; j < l.cols; j++)
            printf("%d\t",l.m[i][j]);
        printf("|%d\n", i);
    }
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
                    printf("~");
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
        printf("Failed to open file\n");
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

Case trouver_entree(Laby l){
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

    if(cases_egales(start, (Case){UNUSED, UNUSED})){
        printf("Impossible de trouver l'entree\n");
        exit(1);
    }
    return start;
}

Case trouver_sortie(Laby l){
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

    if(cases_egales(end, (Case){UNUSED, UNUSED})){
        printf("Impossible de trouver la sortie\n");
        exit(1);
    }
    return end;
}

// ================================================================= fonctions de la P1

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
    if(current.line-1 >= 0      && !Case_in_chemin(current.col, current.line-1, res) && l.m[current.col][current.line-1] != MUR && l.m[current.col][current.line-1] !=  VISITE) // left
        rec_find(l, res, (Case){current.col, current.line-1}, end);
    if(current.col - 1 >= 0     && !Case_in_chemin(current.col-1, current.line, res) && l.m[current.col-1][current.line] != MUR && l.m[current.col-1][current.line] !=  VISITE) // up
        rec_find(l, res, (Case){current.col-1, current.line}, end);   
    if(current.line+1 < l.cols  && !Case_in_chemin(current.col, current.line+1, res) && l.m[current.col][current.line+1] != MUR && l.m[current.col][current.line+1] !=  VISITE) // down
        rec_find(l, res, (Case){current.col, current.line+1}, end);
    if(current.col+1 < l.lignes && !Case_in_chemin(current.col+1, current.line, res) && l.m[current.col+1][current.line] != MUR && l.m[current.col+1][current.line] !=  VISITE) // right
        rec_find(l, res, (Case){current.col+1, current.line}, end);
}

chemin solve_labyrinth(Laby l){

    Case start = trouver_entree(l);
    Case end = trouver_sortie(l);
    
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