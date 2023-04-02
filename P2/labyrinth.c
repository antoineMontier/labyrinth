#include "labyrinth.h"  

Thread_args * global_args;
pthread_mutex_t acces_ids = PTHREAD_MUTEX_INITIALIZER; // acces aux identifiants de threads et leur chemin associe
pthread_mutex_t acces_laby = PTHREAD_MUTEX_INITIALIZER; // acces au labyrinthe (lecture et ecriture de cases)
pthread_mutex_t acces_out = PTHREAD_MUTEX_INITIALIZER; // acces au stdout, pour ordonner les sorties
pthread_mutex_t acces_ids_history = PTHREAD_MUTEX_INITIALIZER; // acces a l'historique des threads
pthread_mutex_t solution_trouvee = PTHREAD_MUTEX_INITIALIZER; // bloque tant qu'une solution n'est pas trouvee


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
    if(cases_egales(global_args->res[thread_index][0], CASE_NULLE))
        return -1;
    for(int i = 1 ; i < CHEMIN_LENGTH ; ++i)
        if(cases_egales(global_args->res[thread_index][i], CASE_NULLE))
            return i-1;
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
    // allouer la struct partagee par les threads
    global_args = malloc(sizeof(Thread_args));
    // == donner l'acces en lecture/ecriture au labyrinth
    global_args->l = l;
    // == donner l'acces en lecture a la case sortie
    global_args->end = end;
    // == allouer le vecteur de chemins
    global_args->res = malloc(NB_THREAD*sizeof(chemin));
    for(int i = 0 ; i < NB_THREAD ; i++){
        // == allouer chaque chemin du vecteur
        global_args->res[i] = malloc(CHEMIN_LENGTH*sizeof(chemin));
        // == initialiser les chemins
        for(int j = 1 ; j < CHEMIN_LENGTH ; j++)
            global_args->res[i][j] = CASE_NULLE;
        // == ajouter la case depart a chaque vecteur de chemins
        global_args->res[i][0] = (Case){start.col, start.line};
    }
    // == allouer le stockage des threads (threads utilises simultanement)
    global_args->threads = malloc(NB_THREAD*sizeof(pthread_t));
    // == initialiser pour ne pas avoir de mauvaise surprise
    for(int i = 0 ; i < NB_THREAD ; i++)
        global_args->threads[i] = 0;
    // == allouer le stockage de l'historique des threads (tous les threads utilises) ; sert a s'assurer lors du join que tout le monde a bien fini
    global_args->threads_history = malloc(NB_THREAD_TOTAL * sizeof(pthread_t));
    // == initialiser pour ne pas avoir de mauvaise surprise
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

chemin resoudre_avec_threads(Laby l){

    // si NB_THREAD == 1, lancer en recursif normal
    if(NB_THREAD <= 1) return resoudre_recursivement(l);

    Case start = trouver_entree(l);
    Case end = trouver_sortie(l);  
    l.m[start.col][start.line] = VISITE;
    allouer_arguments(&l, start, end);
    
    // == allouer le chemin final
    chemin reponse_finale = malloc(CHEMIN_LENGTH * sizeof(Case));
    // == initialiser
    for(int i = 0; i < CHEMIN_LENGTH; i++)
        reponse_finale[i] = CASE_NULLE;

    // == mettre le mutex de solution trouvee a lock (car pas de solution encore trouvee)
    pthread_mutex_lock(&solution_trouvee);


    pthread_create(&(global_args->threads[0]), NULL, (void*)recursivite_thread, NULL); // lancer la fonction dans un thread
    // directement enregister le thread dans lequel la fonction a ete lancee
    ajouter_dans_historique(global_args->threads[0]);


    pthread_mutex_lock(&solution_trouvee); // faire attendre le thread principale

    // join les threads (les chemins parcourus restent en memoire)
    for(int i = 0 ; i < NB_THREAD_TOTAL ; ++i)
        if(global_args->threads_history[i] > 0){
            pthread_join(global_args->threads_history[i], NULL);
            global_args->threads_history[i] = -1;
        }    

    // lire le chemin reponse dans le tableau de chemin : 

    for(int i = 0 ; i < NB_THREAD ; ++i){
        if(cases_egales(global_args->res[i][indiceDeDerniereCase(i)], end)) { // chemin directement trouvé 
            for(int j = 0 ; j < CHEMIN_LENGTH ; ++j)
                reponse_finale[j] = global_args->res[i][j];
            i = NB_THREAD; // stop boucle
        }else if(i == NB_THREAD)
            printf("Erreur: pas de chemin retenu\n");
    }

    // rendre l'espace utilisé
    free_arguments();

    // remettre les cases depart & arrivee (peuvent etre malencontreusement remplacees par des cases VISITE lors de la recursivite)
    l.m[start.col][start.line] = ENTREE; l.m[end.col][end.line] = SORTIE;

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
    if(t_id == -1){ // gestion d'erreur
        print("erreur, identifiant de thread == -1 dans la fonction est_dans_un_cul_de_sac");
        exit(1);
    }
    for(int i = 0 ;  i < CHEMIN_LENGTH && !cases_egales(global_args->res[t_id][i], CASE_NULLE) ; i++)
        if(une_possibilites_de_mouvement(global_args->res[t_id][i])) return 0; // pas de cul de sac
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

void recursivite_thread(){
    //print_ids(); // affiche les thread_t ainsi que le chemin qu'ils ont parcouru
    // ================ ARRET : solution trouvee =================
    if(pthread_mutex_trylock(&solution_trouvee) == 0) {pthread_mutex_unlock(&solution_trouvee); pthread_exit(NULL);}

    // ================ Recuperer le n° de thread, la case sur laquelle il se trouve et les caracteristiques de cette case =================
    pthread_mutex_lock(&acces_ids); // ======= lock

    int thread_index = get_thread_index(); // n° de thread
    int case_index = indiceDeDerniereCase(thread_index); // n° de case
    if(case_index >= CHEMIN_LENGTH - 2){
        print("pas assez de memoire allouee au vecteur de chemins, augmenter #define CHEMIN_LENGTH");
        exit(1);
    }
    int ln = global_args->res[thread_index][case_index].line, cl = global_args->res[thread_index][case_index].col; // obtenir n° de ligne et de colonne
    pthread_mutex_unlock(&acces_ids); // ===== unlock

    // ================ ARRET : je suis sur la case end ====================
    if(cases_egales((Case){cl, ln}, global_args->end)){
        pthread_mutex_lock(&acces_ids); // lock ici pour attendre que tout le mode ecrive dans sa memoire associee avant au lieu d'arreter brutalement les autres threads : eviter les segfault
        pthread_mutex_unlock(&acces_ids);
        pthread_mutex_unlock(&solution_trouvee); // debloquer pour signaler que la solution est trouvee
        pthread_exit(NULL);
    }
    // ============== CHECK DIRECTIONS ========================= Je nai commente que pour cette direction, les 4 directions ont le meme code, elles different par les cases dans laquelle la recurtisvite / thread est lance.
    if(ln-1 >= 0){
        pthread_mutex_lock(&acces_laby);       // lock le labyrinth pour ne pas se melanger avec les autres threads (exemple d'un thread qui voudrait visiter la même case en meme temps, les deux voient qulle est libre... et cest le bazar)
        if(global_args->l->m[cl][ln-1] ==  LIBRE || global_args->l->m[cl][ln-1] ==  SORTIE){ // verifier que c'est possible d'xplorer la case ( condition equivalente a [global_args->l->m[cl][ln-1] !=  MUR && global_args->l->m[cl][ln-1] !=  VISITE])
            global_args->l->m[cl][ln-1] = VISITE; // marquer la case comme visitee
            pthread_mutex_unlock(&acces_laby); // liberer lacces au labyrinth
            pthread_mutex_lock(&acces_ids); // bloquer l'acces a la memoire des identifiants de threads ainsi que leur memoires correspondant aux chemins
            int indice_libre = indiceDunePlaceLibre(); // retourne un indice libre. si pas d'indice libre, retourne -1
            if(!une_possibilites_de_mouvement((Case){cl, ln}) ||  indice_libre == -1){ // indice_libre == -1 signifie que le nombre max de thread est atteint
                ajouter_coord_et_nettoyer_apres(cl, ln-1, global_args->res[thread_index]); // nettoie toutes les cases apres celle que l'on vient d'ajouter. la case que l'on vient d'ajouter a ete ajoutee a cote d'une case voisine (selon la logique du backtrack)
                pthread_mutex_unlock(&acces_ids); // deverouiller l'acces memoire des que possible
                recursivite_thread(); // lancer la recursivite
            }else{
                copier_chemins(thread_index, indice_libre); // copier le chemin parcouru pour que le nv thread sache où il est et en cas de solution, il puisse donner le chemin entier
                ajouter_coord_et_nettoyer_apres(cl, ln-1, global_args->res[indice_libre]); // ajouter la case suivante au vecteur de la meme maniere que dans la fonction recursive
                pthread_create(&(global_args->threads[indice_libre]), NULL, (void*)recursivite_thread, NULL); // lancer le thread, grace aux varaibles partagees, pas besoin d'arguments ni de valeurs de retour
                pthread_mutex_unlock(&acces_ids); // apres (pas avant car le lancement de thread ecrit dans la memoire la valeur du pthread_t cree) on libere l'acces memoire.
                ajouter_dans_historique(global_args->threads[indice_libre]); // on ajoute dans l'historique juste apres.
                }
        }else pthread_mutex_unlock(&acces_laby); // liberer l'acces au labyrinth si on n'est pas rentre dans le if (pas de chemin possible...)
    }if(cl-1 >= 0){
        pthread_mutex_lock(&acces_laby);    
        if(global_args->l->m[cl-1][ln] ==  LIBRE || global_args->l->m[cl-1][ln] ==  SORTIE){
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
        if(global_args->l->m[cl][ln+1] ==  LIBRE || global_args->l->m[cl][ln+1] ==  SORTIE){
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
        if(global_args->l->m[cl+1][ln] ==  LIBRE || global_args->l->m[cl+1][ln] ==  SORTIE){
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

Laby creer_labyrinth(int cols, int lines){
    if(cols < 4){
        printf("erreur : il faut que cols soit superieur ou egal à 5, vous avez passe %d\n",cols);
        exit(1);
    }
    if(lines < 4){
        printf("erreur : il faut que lines soit superieur ou egal à 5, vous avez passe %d\n",lines);
        exit(1);
    }
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
        fgetc(out); // ne pas prendre en compte le '\n'
    }
    
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

Case trouver_entree(Laby l){
    // trouvons le depart
    Case start = CASE_NULLE;
    for(int i=0; i< l.lignes; i++)
        for(int j=0; j< l.cols; j++)
            if(l.m[i][j] == ENTREE){
                start.col = i;
                start.line = j;
                //arret des boucles
                i = l.lignes; j = l.cols;
            }

    if(cases_egales(start, CASE_NULLE)){
        printf("Impossible de trouver l'entree\n");
        exit(1);
    }
    return start;
}

Case trouver_sortie(Laby l){
    // trouvons l'arivee
    Case end = CASE_NULLE;
    for(int i=0; i< l.lignes; i++)
        for(int j=0; j< l.cols; j++)
            if(l.m[i][j] == SORTIE){
                end.col = i;
                end.line = j;
                //arret des boucles
                i = l.lignes; j = l.cols;
            }

    if(cases_egales(end, CASE_NULLE)){
        printf("Impossible de trouver la sortie\n");
        exit(1);
    }
    return end;
}

void free_labyrinth(Laby*l){
    if(l->m == NULL) return; // rien a faire

    for(int i = 0 ; i < l->lignes ; i++)
        if(l->m[i] != NULL) free(l->m[i]);

    if(l->m != NULL) free(l->m);

    l->m = NULL;
}

int verifier_solution(Laby l, chemin Case_tab){
    //verifier ENTREE
    if(l.m[Case_tab[0].col][Case_tab[0].line] != ENTREE) return 0; //false

    int index_end;
    //avancer index_end jusqu'au cases inutilisees
    for(index_end = 0; index_end < CHEMIN_LENGTH-2 ; index_end++)
        if(cases_egales(Case_tab[index_end+1], CASE_NULLE))
            break;

    if(index_end == CHEMIN_LENGTH - 1) return 0; // pas de fin de chemin

    //verifier que les cases sont voisines
    for(int i = 1; i <= index_end ; i++)
        if(!sont_voisines(Case_tab[i], Case_tab[i-1])) return 0;

    return 1; // tous tests passes
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
                case ENTREE:
                    printf("+");
                    break;
                case SORTIE:
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

void nettoyer_chemin(chemin c){
    int ind_fin = CHEMIN_LENGTH-2; // se placer à l'indice fin
    while(ind_fin >= 0 && cases_egales(c[ind_fin], CASE_NULLE)) ind_fin--;

    if(ind_fin == 0) return;// chemin n'a aucune case valable, rien à nettoyer
    
    while(ind_fin >=0 && !sont_voisines(c[ind_fin], c[ind_fin-1])){ // chercher le premier voisin en continuant de remonter vers le depart
        c[ind_fin -1] = c[ind_fin];
        c[ind_fin] = CASE_NULLE;
        --ind_fin;
    }
}

void recursivite_simple(Laby l, chemin res, Case current, Case end){
    if(res == NULL){
        printf("chemin doit etre declare avec malloc\n");
        exit(1);
    }

    if(res[CHEMIN_LENGTH-1].col == END_SIGNAL && res[CHEMIN_LENGTH-1].line == END_SIGNAL) return; // une solution a deja ete trouvee
    if(cases_egales(current, end)){
        // ajouter à la main la derniere case dans le chemin :
        for(int i=0; i<CHEMIN_LENGTH; i++)
            if(cases_egales(res[i], CASE_NULLE)){
                res[i] = end;
                break;
            }
        res[CHEMIN_LENGTH-1] = (Case){END_SIGNAL, END_SIGNAL};
        return;
    }

    // marquer la case comme visitée : 
    l.m[current.col][current.line] = VISITE;

    //ajouter la case dans le chemin
    ajouter_coord_et_nettoyer_apres(current.col, current.line, res);

    // verifier les 4 directions
    if(current.line-1 >= 0      && !caseDansChemin(current.col, current.line-1, res) && l.m[current.col][current.line-1] != MUR && l.m[current.col][current.line-1] !=  VISITE) recursivite_simple(l, res, (Case){current.col, current.line-1}, end);
    if(current.col - 1 >= 0     && !caseDansChemin(current.col-1, current.line, res) && l.m[current.col-1][current.line] != MUR && l.m[current.col-1][current.line] !=  VISITE) recursivite_simple(l, res, (Case){current.col-1, current.line}, end);   
    if(current.line+1 < l.cols  && !caseDansChemin(current.col, current.line+1, res) && l.m[current.col][current.line+1] != MUR && l.m[current.col][current.line+1] !=  VISITE) recursivite_simple(l, res, (Case){current.col, current.line+1}, end);
    if(current.col+1 < l.lignes && !caseDansChemin(current.col+1, current.line, res) && l.m[current.col+1][current.line] != MUR && l.m[current.col+1][current.line] !=  VISITE) recursivite_simple(l, res, (Case){current.col+1, current.line}, end);
}

chemin resoudre_recursivement(Laby l){
    Case start = trouver_entree(l);
    Case end = trouver_sortie(l);
    chemin reponse = malloc(sizeof(chemin) * CHEMIN_LENGTH);
    
    for(int i = 0; i < CHEMIN_LENGTH; i++)
        reponse[i] = CASE_NULLE;

    // lancer la recursivite
    recursivite_simple(l, reponse, start, end);

    //remettre la case depart : 
    l.m[start.col][start.line] = ENTREE;

    // nettoyer la reponse si necessaire : 
    if(!verifier_solution(l, reponse)) nettoyer_chemin(reponse);

    return reponse;
}