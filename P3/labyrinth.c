#include "labyrinth.h"  

Thread_args * global_args;
pthread_mutex_t acces_ids = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t acces_laby = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t acces_out = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t acces_ids_history = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t solution_trouvee = PTHREAD_MUTEX_INITIALIZER;

chemin lire_fichier(const char* filename) {
    chemin chemin = NULL;
    int case_count = 0;
    FILE* fp = fopen(filename, "r");

    if (fp == NULL) {
        printf("impossible d'ouvrir le fichier '%s'\n", filename);
        return NULL;
    }
    char buffer[8*CHEMIN_LENGTH];

    chemin = malloc(CHEMIN_LENGTH * sizeof(Case));
    // Read cases from file
    while (fgets(buffer, sizeof(buffer), fp) != NULL) {
        char* token = strtok(buffer, "|");
        while (token != NULL) {
            int col, line;
            if (sscanf(token, "%d %d", &col, &line) == 2) {
                chemin[case_count].col = col;
                chemin[case_count].line = line;
                case_count++;
            } else {
                printf("Erreur avec le token '%s' > augmenter la taille du chemin (celle du buffer en depend) peut solutionner le probleme\n", token);
            }
            token = strtok(NULL, "|");
        }
    }
    fclose(fp);
    return chemin;
}

chemin* course_de_process(Laby l){
    if(NB_THREAD < 3){
        printf("Erreur : allouer au moins 4 Threads\n");
        return NULL;
    }
    // ==== trouver start_1/2, porte, end_1/2
    Case start_1 = trouver_entree_1(l);
    Case start_2 = trouver_entree_2(l);
    Case end_1 = trouver_sortie_1(l);
    Case end_2 = trouver_sortie_2(l);
    Case porte = trouver_porte(l);
    // mettre en LIBRE pour permettre au joueur 2 de visiter l'entree et sortie du joueur 1
    l.m[start_1.col][start_1.line] = l.m[start_2.col][start_2.line] = l.m[end_2.col][end_2.line] = l.m[end_1.col][end_1.line] = l.m[porte.col][porte.line] = LIBRE;

    chemin ch;
    pid_t son = fork();

    // ==== lancer entre ENTREE_1 et PORTE
    if(son > 0)
        ch = resoudre_avec_threads(l, start_1, porte);
    // ==== lancer entre ENTREE_2 et PORTE
    if(son == 0)
        ch = resoudre_avec_threads(l, start_2, porte);

    // ==== ecrire le chemin parcouru entre ENTREE_1/2 et PORTE
    if(son > 0){
        FILE*result = fopen("a1.res", "w");
        if(result == NULL){
            printf("erreur ouverture fichier resultat n°1\n");
            wait(NULL);
            exit(1);
        }
        for(int i = 1 ; i < CHEMIN_LENGTH && !cases_egales(ch[i], CASE_NULLE) ; ++i)
            fprintf(result, "%d %d|", ch[i-1].col, ch[i-1].line);
        fclose(result);
        free(ch);
        nettoie_matrice(l); // retirer les cases visitees pour permettre de rebrousser chemin
        // === attendre que le fork ait fini
        FILE*attente;
        do{
            attente = fopen("a2.res", "r");
        }while(attente == NULL);
        fclose(attente); // continuer
    }

    if(son == 0){        
        // === attendre que le main ait fini

        FILE*result = fopen("a2.res", "w");
        if(result == NULL){
            printf("erreur ouverture fichier resultat n°2\n");
            exit(1);
        }
        for(int i = 1 ; i < CHEMIN_LENGTH && !cases_egales(ch[i], CASE_NULLE) ; ++i)
            fprintf(result, "%d %d|", ch[i-1].col, ch[i-1].line);
        fclose(result);
        free(ch);        
        nettoie_matrice(l);
        FILE*attente;
        do{
            attente = fopen("a1.res", "r");
        }while(attente == NULL);
        fclose(attente); // continuer
    }


    // ==== lancer entre PORTE et SORTIE_1/2
    if(son > 0)
        ch = resoudre_avec_threads(l, porte, end_1);
    // ==== lancer entre ENTREE_2 et PORTE
    if(son == 0)
        ch = resoudre_avec_threads(l, porte, end_2);

    // ==== attendre les resultats

    if(son > 0){
        FILE*result = fopen("a1.res", "a");
        if(result == NULL){
            printf("erreure dans l'ouverture fichier resultat n°1\n");
            wait(NULL);
            exit(1);
        }
        for(int i = 0 ; i < CHEMIN_LENGTH && !cases_egales(ch[i], CASE_NULLE) ; ++i)
            fprintf(result, "%d %d|", ch[i].col, ch[i].line);
        fclose(result);
        free(ch);
    }


    if(son == 0){
        FILE*result = fopen("a2.res", "a");
        if(result == NULL){
            printf("erreure dans l'ouverture fichier resultat n°2\n");
            exit(1);
        }
        for(int i = 0 ; i < CHEMIN_LENGTH && !cases_egales(ch[i], CASE_NULLE) ; ++i)
            fprintf(result, "%d %d|", ch[i].col, ch[i].line);
        fclose(result);
        free(ch);
        exit(0);
    }

    // ==== wait 
    wait(NULL);
    // ici il n'y a plus que le fork initial

    // ==== lire les chemins
    chemin*reponse = malloc(2*sizeof(chemin));
    reponse[0] = lire_fichier("a1.res");
    reponse[1] = lire_fichier("a2.res");

    // ==== suppr les fichiers crees
    system("rm a1.res a2.res");

    // ==== remettre les cases comme elles etaient
    l.m[start_1.col][start_1.line] = ENTREE_1;
    l.m[start_2.col][start_2.line] = ENTREE_2;
    l.m[end_2.col][end_2.line] = SORTIE_2;
    l.m[end_1.col][end_1.line] = SORTIE_1;
    l.m[porte.col][porte.line] = PORTE;

    return reponse;
}

void nettoie_matrice(Laby l){
    for(int i=0; i < l.lignes; ++i)
        for(int j=0; j < l.cols; ++j)
            if(l.m[i][j] == VISITE)
                l.m[i][j] = LIBRE;
}

void print_ids(){
    pthread_mutex_lock(&acces_out);
    printf("\n\n=================Threads_ids :\n");
    for(int i =  0; i < NB_THREAD ; ++i){
        printf("%p --> ", (void*)global_args->threads[i]);
        print_chemin(global_args->res[i]);
    }
    printf("=================\n\n");
    pthread_mutex_unlock(&acces_out);
}

int indiceDeDerniereCase(int thread_index){
    if(thread_index == -1){
        print("indiceDeDerniereCase error, thread index == -1");
        exit(1);
    }
    if(cases_egales(global_args->res[thread_index][0], CASE_NULLE)) return -1;
    for(int i = 1 ; i < CHEMIN_LENGTH ; ++i)
        if(cases_egales(global_args->res[thread_index][i], CASE_NULLE)) return i-1;
    return CHEMIN_LENGTH;
}

void ajouter_dans_historique(pthread_t thread_id){
    pthread_mutex_lock(&acces_ids_history);
    for(int i = 0 ; i < NB_THREAD_TOTAL ; ++i){
        if(global_args->threads_history[i] == thread_id){
            pthread_mutex_unlock(&acces_ids_history);
            return; // deja present
        }if(global_args->threads_history[i] == 0){
            global_args->threads_history[i] = thread_id;
            pthread_mutex_unlock(&acces_ids_history);
            return;
        }
        if(i == NB_THREAD_TOTAL){
            print("Erreur: nombre maximal de thread dans l'historique atteint, augmenter le #define NB_THREAD_TOTAL");
            exit(1);
        }
    }
}

void allouer_arguments(Laby *l, Case start, Case end){
    global_args = malloc(sizeof(Thread_args));
    global_args->l = l;
    global_args->end = end;
    global_args->res = malloc(NB_THREAD*sizeof(chemin));
    for(int i = 0 ; i < NB_THREAD ; i++){
        global_args->res[i] = malloc(CHEMIN_LENGTH*sizeof(chemin));
        for(int j = 1 ; j < CHEMIN_LENGTH ; j++)
            global_args->res[i][j] = CASE_NULLE;
        global_args->res[i][0] = (Case){start.col, start.line};
    }
    global_args->threads = malloc(NB_THREAD*sizeof(pthread_t));
    for(int i = 0 ; i < NB_THREAD ; i++)
        global_args->threads[i] = 0;
    global_args->threads_history = malloc(NB_THREAD_TOTAL * sizeof(pthread_t));
    for(int i = 0 ; i < NB_THREAD_TOTAL ; i++)
        global_args->threads_history[i] = 0;
}

void free_arguments(){
    for(int i = 0 ; i < NB_THREAD ; ++i)
        free(global_args->res[i]);
    free(global_args->res);
    free(global_args->threads);
    free(global_args->threads_history);
    free(global_args);    
}

chemin resoudre_avec_threads(Laby l, Case start, Case end){
    l.m[start.col][start.line] = VISITE;
    l.m[end.col][end.line] = LIBRE;

    allouer_arguments(&l, start, end);

    chemin reponse_finale = malloc(CHEMIN_LENGTH * sizeof(Case));
    for(int i = 0; i < CHEMIN_LENGTH; i++) reponse_finale[i] = CASE_NULLE;

    pthread_mutex_unlock(&solution_trouvee); // debloquer pour le second tour au cas ou le mutex est reste bloque entre ENTREE_1/2 et PORTE
    pthread_mutex_lock(&solution_trouvee);

    pthread_create(&(global_args->threads[0]), NULL, (void*)recursivite_thread, NULL);

    ajouter_dans_historique(global_args->threads[0]);
    pthread_mutex_lock(&solution_trouvee);

    for(int i = 0 ; i < NB_THREAD_TOTAL ; ++i)
        if(global_args->threads_history[i] > 0){
            pthread_join(global_args->threads_history[i], NULL);
            global_args->threads_history[i] = -1;
        }    
    for(int i = 0 ; i < NB_THREAD ; ++i){
        if(cases_egales(global_args->res[i][indiceDeDerniereCase(i)], end)) {
            for(int j = 0 ; j < CHEMIN_LENGTH ; ++j)
                reponse_finale[j] = global_args->res[i][j];
            i = NB_THREAD;
        }else if(i == NB_THREAD)
            printf("Erreur: pas de chemin retenu\n");
    }
    free_arguments();
    return reponse_finale;
}

int get_thread_index(){
    for(int i = 0 ; i < NB_THREAD ; ++i)
        if(global_args->threads[i] == pthread_self())
            return i;
    return -1;
}

int indiceDunePlaceLibre(){
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

int une_possibilites_de_mouvement(Case c){
    if(cases_egales(c, CASE_NULLE))
        return 0;
    int temp; // utilisation de temp pour ne pas acceder 2 fois a la meme case de la matrice
    if(c.line-1 >= 0 && (temp = global_args->l->m[c.col][c.line-1]) != MUR && temp != VISITE) return 1;
    if(c.col-1 >= 0 && (temp = global_args->l->m[c.col-1][c.line]) != MUR && temp != VISITE) return 1;
    if(c.line+1 < global_args->l->cols && (temp = global_args->l->m[c.col][c.line+1]) != MUR && temp != VISITE) return 1;
    if(c.col+1 < global_args->l->lignes && (temp = global_args->l->m[c.col+1][c.line]) != MUR && temp != VISITE) return 1;
    return 0;
}

void print_solution(Laby l, chemin c){
    // ajouter des lettres dans le labyrinth pour montrer le chemin pris
    int ch = 'a';
    for(int i=1; i<CHEMIN_LENGTH-1 ; ++i){
        if(cases_egales(c[i+1], CASE_NULLE)) break;
        else l.m[c[i].col][c[i].line] = ch++;//'@'; // affichage du chemin avec les characteres ascii a,b->z,A,B->Z
        if(ch == 'z' + 1) ch = 'A';
        if(ch == 'Z' + 1) ch = 'a';
        }

    print_labyrinth(l); // afficher le labyrinth avec les modifications faites (qui seront prises en comptes par le default du switch)

    // retirer les lettres ensuite
    for(int i=1; i < CHEMIN_LENGTH - 1 ; ++i){
        if(cases_egales(c[i+1], CASE_NULLE)) break;
        else l.m[c[i].col][c[i].line] = LIBRE;
        }
}

int est_dans_un_cul_de_sac(int t_id){
    if(t_id == -1){
        print("erreur, identifiant de thread == -1 dans la fonction est_dans_un_cul_de_sac");
        exit(1);
    }
    for(int i = 0 ;  i < CHEMIN_LENGTH && !cases_egales(global_args->res[t_id][i], CASE_NULLE) ; i++)
        if(une_possibilites_de_mouvement(global_args->res[t_id][i])) return 0;
    return 1;
}

void print_chemin(chemin c){
    printf("[");
    for(int i = 0; i < CHEMIN_LENGTH ; i++){
        if(cases_egales(c[i], CASE_NULLE)) break;
        printf(" %d;%d ", c[i].col, c[i].line);
    }
    printf("]\n");
}

int ajouter_coord_et_nettoyer_apres(int col, int line, chemin c){
    if(col == UNUSED || line == UNUSED){
        print("erreur dans le format de la case");
        return 0;
    }
    Case s = {col, line};
    if(cases_egales(c[0], CASE_NULLE)){
        c[0] = s;
        for(int j = 1 ; !cases_egales(c[j], CASE_NULLE) && j < CHEMIN_LENGTH ; ++j)
            c[j] = CASE_NULLE;        
        return 1;
    }
    for(int i = CHEMIN_LENGTH -2 ; i >= 0 ; --i)
        if(!cases_egales(c[i], CASE_NULLE))
            if(sont_voisines(c[i], s)){
                c[i+1] = s; // case ajoutee
                for(int j = i + 2 ; !cases_egales(c[j], CASE_NULLE) && j < CHEMIN_LENGTH ; ++j)
                    c[j] = CASE_NULLE;
                return 1;
            }
    printf("echec d'ajout de la case {%d, %d}", col, line); print("");
    print_ids();
    return 0;// erreur aucune case ajoutee
}

int caseDansChemin(int col, int line, chemin c){
    int a, b; // utiliser des variables temporaires pour plus d'efficacité ?
    for(int i=0; i < CHEMIN_LENGTH; ++i){
        if((a = c[i].col) == UNUSED || (b = c[i].line) == UNUSED) return 0;
        if(a == col && b == line) return 1;
    }
    return 0;
}

void reset_id_et_chemin(int thread_indice, int derniere_case){
    if(thread_indice >= NB_THREAD || thread_indice < 0){
        print("erreur dans la fonction reset_id_et_chemin, thread_indice no valide");
        return;
    }
    pthread_mutex_lock(&acces_ids);
    for(int i = 0 ; i <= derniere_case + 1  ; ++i )
        global_args->res[thread_indice][i] = CASE_NULLE;
    global_args->threads[thread_indice] = 0;
    pthread_mutex_unlock(&acces_ids);
}

void recursivite_thread(){
    //print_ids(); // affiche les thread_t ainsi que le chemin qu'ils ont parcouru
    // ================ ARRET : solution trouvee =================
    if(pthread_mutex_trylock(&solution_trouvee) == 0) {pthread_mutex_unlock(&solution_trouvee); pthread_exit(NULL);}
    // ================ Recuperer le n° de thread, la case sur laquelle il se trouve et les caracteristiques de cette case =================
    pthread_mutex_lock(&acces_ids);
    int thread_index = get_thread_index(), case_index = indiceDeDerniereCase(thread_index); 
    if(case_index >= CHEMIN_LENGTH - 2){
        print("pas assez de memoire allouee au vecteur de chemins, augmenter #define CHEMIN_LENGTH");
        exit(1);
    }
    int ln = global_args->res[thread_index][case_index].line, cl = global_args->res[thread_index][case_index].col;
    pthread_mutex_unlock(&acces_ids);
    // ================ ARRET : je suis sur la case end ====================
    if(cases_egales((Case){cl, ln}, global_args->end)){
        pthread_mutex_lock(&acces_ids);
        pthread_mutex_unlock(&acces_ids);
        pthread_mutex_unlock(&solution_trouvee);
        pthread_exit(NULL);
    }
    // ============== CHECK DIRECTIONS ========================= Je nai commente que pour cette direction, les 4 directions ont le meme code, elles different par les cases dans laquelle la recurtisvite / thread est lance.
    if(ln-1 >= 0){
        pthread_mutex_lock(&acces_laby);
        if(global_args->l->m[cl][ln-1] ==  LIBRE || global_args->l->m[cl][ln-1] ==  SORTIE_1 || global_args->l->m[cl][ln-1] ==  SORTIE_2){
            global_args->l->m[cl][ln-1] = VISITE;
            pthread_mutex_unlock(&acces_laby);
            pthread_mutex_lock(&acces_ids);
            int indice_libre = indiceDunePlaceLibre();
            if(!une_possibilites_de_mouvement((Case){cl, ln}) ||  indice_libre == -1){
                ajouter_coord_et_nettoyer_apres(cl, ln-1, global_args->res[thread_index]);
                pthread_mutex_unlock(&acces_ids);
                recursivite_thread();
            }else{
                copier_chemins(thread_index, indice_libre);
                ajouter_coord_et_nettoyer_apres(cl, ln-1, global_args->res[indice_libre]);
                pthread_create(&(global_args->threads[indice_libre]), NULL, (void*)recursivite_thread, NULL);
                pthread_mutex_unlock(&acces_ids);
                ajouter_dans_historique(global_args->threads[indice_libre]);
                }
        }else pthread_mutex_unlock(&acces_laby);
    }if(cl-1 >= 0){
        pthread_mutex_lock(&acces_laby);    
        if(global_args->l->m[cl-1][ln] ==  LIBRE || global_args->l->m[cl-1][ln] ==  SORTIE_1 || global_args->l->m[cl-1][ln] ==  SORTIE_2){
            global_args->l->m[cl-1][ln] = VISITE;
            pthread_mutex_unlock(&acces_laby);
            pthread_mutex_lock(&acces_ids);
            int indice_libre = indiceDunePlaceLibre();
            if(!une_possibilites_de_mouvement((Case){cl, ln}) ||  indice_libre == -1){
                ajouter_coord_et_nettoyer_apres(cl-1, ln, global_args->res[thread_index]);
                pthread_mutex_unlock(&acces_ids);
                recursivite_thread();
            }else{
                copier_chemins(thread_index, indice_libre);
                ajouter_coord_et_nettoyer_apres(cl-1, ln, global_args->res[indice_libre]);
                pthread_create(&(global_args->threads[indice_libre]), NULL, (void*)recursivite_thread, NULL);
                pthread_mutex_unlock(&acces_ids);
                ajouter_dans_historique(global_args->threads[indice_libre]);
            }
        }else pthread_mutex_unlock(&acces_laby);
    }if(ln+1 < global_args->l->cols){
        pthread_mutex_lock(&acces_laby);    
        if(global_args->l->m[cl][ln+1] ==  LIBRE || global_args->l->m[cl][ln+1] ==  SORTIE_1 || global_args->l->m[cl][ln+1] ==  SORTIE_2){
            global_args->l->m[cl][ln+1] = VISITE;
            pthread_mutex_unlock(&acces_laby);
            pthread_mutex_lock(&acces_ids);
            int indice_libre = indiceDunePlaceLibre();
            if(!une_possibilites_de_mouvement((Case){cl, ln}) ||  indice_libre == -1){
                ajouter_coord_et_nettoyer_apres(cl, ln+1, global_args->res[thread_index]);
                pthread_mutex_unlock(&acces_ids);
                recursivite_thread();
            }else{
                copier_chemins(thread_index, indice_libre);
                ajouter_coord_et_nettoyer_apres(cl, ln+1, global_args->res[indice_libre]);
                pthread_create(&(global_args->threads[indice_libre]), NULL, (void*)recursivite_thread, NULL);
                pthread_mutex_unlock(&acces_ids);
                ajouter_dans_historique(global_args->threads[indice_libre]);
            }
        }else pthread_mutex_unlock(&acces_laby);
    }if(cl+1 < global_args->l->lignes){
        pthread_mutex_lock(&acces_laby);    
        if(global_args->l->m[cl+1][ln] ==  LIBRE || global_args->l->m[cl+1][ln] ==  SORTIE_1 || global_args->l->m[cl+1][ln] ==  SORTIE_2){
            global_args->l->m[cl+1][ln] = VISITE;
            pthread_mutex_unlock(&acces_laby);
            pthread_mutex_lock(&acces_ids);
            int indice_libre = indiceDunePlaceLibre();
            if(!une_possibilites_de_mouvement((Case){cl, ln}) ||  indice_libre == -1){
                ajouter_coord_et_nettoyer_apres(cl+1, ln, global_args->res[thread_index]);
                pthread_mutex_unlock(&acces_ids);
                recursivite_thread();
            }else{
                copier_chemins(thread_index, indice_libre);
                ajouter_coord_et_nettoyer_apres(cl+1, ln, global_args->res[indice_libre]);
                pthread_create(&(global_args->threads[indice_libre]), NULL, (void*)recursivite_thread, NULL);
                pthread_mutex_unlock(&acces_ids);
                ajouter_dans_historique(global_args->threads[indice_libre]);
            }
        }else pthread_mutex_unlock(&acces_laby);
    }
    if(est_dans_un_cul_de_sac(thread_index)){
        reset_id_et_chemin(thread_index, case_index);
        pthread_exit(NULL);
    }
}

Laby creer_labyrinth(int cols, int lines){
    if (cols < 4){
        printf("erreur : il faut que cols soit superieur ou egal à 5, vous avez passe %d\n", cols);
        exit(1);
    }
    if (lines < 4){
        printf("erreur : il faut que lines soit superieur ou egal à 5, vous avez passe %d\n", lines);
        exit(1);
    }
    char *command = malloc(128);
    sprintf(command, "python3 generateur.py %d %d > out.txt", lines, cols);
    system(command);
    free(command);
    FILE *out = fopen("out.txt", "r");
    if (out == NULL){
        printf("Failed to open file\n");
        exit(1);
    }

    Laby current_laby;
    current_laby.m = malloc(lines * sizeof(int *));
    for (int i = 0; i < lines; i++)
        current_laby.m[i] = (int *)malloc(cols * sizeof(int));

    int c;

    for (int i = 0; i < lines; i++){
        for (int j = 0; j < cols; j++)
            if ((c = fgetc(out)) != '\n')
                current_laby.m[i][j] = c - '0';
        fgetc(out);
    }

    // placer la deuxieme entree, la deuxime sortie et la porte :
    srand(time(NULL));
    int ii, jj;
    do{
        ii = rand() % lines;
        jj = rand() % cols;
    } while (current_laby.m[ii][jj] != LIBRE);
    current_laby.m[ii][jj] = ENTREE_2;

    do{
        ii = rand() % lines;
        jj = rand() % cols;
    } while (current_laby.m[ii][jj] != LIBRE);
    current_laby.m[ii][jj] = PORTE;

    do{
        ii = rand() % lines;
        jj = rand() % cols;
    } while (current_laby.m[ii][jj] != LIBRE);
    current_laby.m[ii][jj] = SORTIE_2;

    fclose(out);
    system("rm out.txt");
    current_laby.lignes = lines;
    current_laby.cols = cols;
    return current_laby;
}

int abso(int x){return x < 0 ? -x : x;}
int sont_voisines(Case a, Case b){return (a.col == b.col && abso(a.line - b.line) == 1) || (a.line == b.line && abso(a.col - b.col) == 1);}
int cases_egales(Case a, Case b){return a.col == b.col && a.line == b.line;}
void print_Case(Case s){printf("%d %d\n", s.col, s.line);}

Case trouver_entree_1(Laby l){
    // trouvons le depart
    Case start = CASE_NULLE;
    for (int i = 0; i < l.lignes; i++)
        for (int j = 0; j < l.cols; j++)
            if (l.m[i][j] == ENTREE_1){
                start.col = i;
                start.line = j;
                // arret des boucles
                i = l.lignes;
                j = l.cols;
            }

    if (cases_egales(start, CASE_NULLE)){
        printf("Impossible de trouver l'entree\n");
        exit(1);
    }
    return start;
}

Case trouver_sortie_1(Laby l){
    // trouvons l'arivee
    Case end = CASE_NULLE;
    for (int i = 0; i < l.lignes; i++)
        for (int j = 0; j < l.cols; j++)
            if (l.m[i][j] == SORTIE_1){
                end.col = i;
                end.line = j;
                // arret des boucles
                i = l.lignes;
                j = l.cols;
            }

    if (cases_egales(end, CASE_NULLE)){
        printf("Impossible de trouver la sortie\n");
        exit(1);
    }
    return end;
}

Case trouver_entree_2(Laby l){
    // trouvons le depart
    Case start = CASE_NULLE;
    for (int i = 0; i < l.lignes; i++)
        for (int j = 0; j < l.cols; j++)
            if (l.m[i][j] == ENTREE_2){
                start.col = i;
                start.line = j;
                // arret des boucles
                i = l.lignes;
                j = l.cols;
            }

    if (cases_egales(start, CASE_NULLE)){
        printf("Impossible de trouver l'entree\n");
        exit(1);
    }
    return start;
}

Case trouver_sortie_2(Laby l){
    // trouvons l'arivee
    Case end = CASE_NULLE;
    for (int i = 0; i < l.lignes; i++)
        for (int j = 0; j < l.cols; j++)
            if (l.m[i][j] == SORTIE_2){
                end.col = i;
                end.line = j;
                // arret des boucles
                i = l.lignes;
                j = l.cols;
            }

    if (cases_egales(end, CASE_NULLE)){
        printf("Impossible de trouver la sortie\n");
        exit(1);
    }
    return end;
}

Case trouver_porte(Laby l){
    // trouvons le depart
    Case start = CASE_NULLE;
    for (int i = 0; i < l.lignes; i++)
        for (int j = 0; j < l.cols; j++)
            if (l.m[i][j] == PORTE){
                start.col = i;
                start.line = j;
                // arret des boucles
                i = l.lignes;
                j = l.cols;
            }

    if (cases_egales(start, CASE_NULLE)){
        printf("Impossible de trouver l'entree\n");
        exit(1);
    }
    return start;
}

void free_labyrinth(Laby*l){
    if(l->m == NULL) return; // rien a faire

    for(int i = 0 ; i < l->lignes ; i++)
        if(l->m[i] != NULL) free(l->m[i]);

    if(l->m != NULL) free(l->m);

    l->m = NULL;
}

void print_labyrinth(Laby l){
    for(int i = 0 ; i < l.cols ; i++){
        if(i < 10) printf(" ");
        else printf("%d", i/10);
    } 
    printf("|\n");
    for(int i = 0 ; i < l.cols ; i++) printf("%d", i%10);
    printf("|\n");
    for(int i = 0 ; i < l.cols ; i++) printf("-");

    printf("|+\n");
    for(int i=0; i < l.lignes; i++){
        for(int j=0; j < l.cols; j++)
            switch(l.m[i][j]){
                case MUR:
                    printf("#");
                    break;
                case LIBRE:
                    printf(" ");
                    break;
                case ENTREE_1:
                    printf("+");
                    break;
                case ENTREE_2:
                    printf("*");
                    break;
                case PORTE:
                    printf("@");
                    break;
                case SORTIE_1:
                    printf("-");
                    break;
                case SORTIE_2:
                    printf("/");
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

int verifier_solution(Laby l, Case*Case_tab, Case start, Case porte, Case end){
    int porte_index = CHEMIN_LENGTH + 2;
    int end_index = 0;
    if(!cases_egales(Case_tab[0], start)){
        printf("entree pas au debut du chemin\n");
        return 0; //false
    }

    //verifier que les cases sont voisines
    for(int i = 1; i < CHEMIN_LENGTH && !cases_egales(Case_tab[i], CASE_NULLE) ; ++i){
        if(cases_egales(Case_tab[i], end)){
            end_index = i;
            if(porte_index < end_index)
                return 1;
        }
        if(cases_egales(Case_tab[i], porte)) porte_index = i;
        if(!sont_voisines(Case_tab[i], Case_tab[i-1])){
            printf("ne sont pas voisines : %d et %d \n", i-1, i);
            return 0;
        }
    }
    printf("porte = %d, end = %d\n", porte_index, end_index);
    if(end_index > porte_index) return 1;
    return 0;
}
