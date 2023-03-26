#include "labyrinth.h"

Thread_args *global_args_1;
Thread_args *global_args_2;

pthread_mutex_t acces_ids_1 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t acces_ids_2 = PTHREAD_MUTEX_INITIALIZER;

pthread_mutex_t acces_laby_1 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t acces_laby_2 = PTHREAD_MUTEX_INITIALIZER;

pthread_mutex_t acces_ids_history_1 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t acces_ids_history_2 = PTHREAD_MUTEX_INITIALIZER;

pthread_mutex_t solution_trouvee_1 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t solution_trouvee_2 = PTHREAD_MUTEX_INITIALIZER;

pthread_mutex_t acces_out = PTHREAD_MUTEX_INITIALIZER;

void print_ids_1()
{
    pthread_mutex_lock(&acces_out);
    printf("\n\n=================Threads _ids :\n");
    for (int i = 0; i < NB_THREAD; ++i)
    {
        printf("%p -- ", (void *)global_args_1->threads[i]);
        print_chemin(global_args_1->res[i]);
    }
    printf("=================\n\n");
    pthread_mutex_unlock(&acces_out);
}

void print_ids_2()
{
    pthread_mutex_lock(&acces_out);
    printf("\n\n=================Threads _ids :\n");
    for (int i = 0; i < NB_THREAD; ++i)
    {
        printf("%p -- ", (void *)global_args_2->threads[i]);
        print_chemin(global_args_2->res[i]);
    }
    printf("=================\n\n");
    pthread_mutex_unlock(&acces_out);
}

int getLastCaseIndex_1(int thread_index)
{
    if (thread_index == -1)
    {
        print("getLastCaseIndex error, thread index is -1");
        exit(1);
    }
    if (cases_egales(global_args_1->res[thread_index][0], CASE_NULLE))
        return -1;
    for (int i = 1; i < CHEMIN_LENGTH; ++i)
        if (cases_egales(global_args_1->res[thread_index][i], CASE_NULLE))
            return i - 1;
    return CHEMIN_LENGTH;
}

int getLastCaseIndex_2(int thread_index)
{
    if (thread_index == -1)
    {
        print("getLastCaseIndex error, thread index is -1");
        exit(1);
    }
    if (cases_egales(global_args_2->res[thread_index][0], CASE_NULLE))
        return -1;
    for (int i = 1; i < CHEMIN_LENGTH; ++i)
        if (cases_egales(global_args_2->res[thread_index][i], CASE_NULLE))
            return i - 1;
    return CHEMIN_LENGTH;
}

void ajouter_dans_historique_1(pthread_t thread_id)
{
    pthread_mutex_lock(&acces_ids_history_1);
    for (int i = 0; i < NB_THREAD_TOATL; ++i)
    {
        if (global_args_1->threads_history[i] == thread_id)
        {
            pthread_mutex_unlock(&acces_ids_history_1);
            return; // deja present
        }
        if (global_args_1->threads_history[i] == 0)
        {
            global_args_1->threads_history[i] = thread_id;
            pthread_mutex_unlock(&acces_ids_history_1);
            return;
        }
        if (i == NB_THREAD_TOATL)
        {
            print("Erreur: nombre maximal de thread dans l'historique atteint, augmenter le #define NB_THREAD_TOATL");
            exit(1);
        }
    }
}

void ajouter_dans_historique_2(pthread_t thread_id)
{
    pthread_mutex_lock(&acces_ids_history_2);
    for (int i = 0; i < NB_THREAD_TOATL; ++i)
    {
        if (global_args_2->threads_history[i] == thread_id)
        {
            pthread_mutex_unlock(&acces_ids_history_2);
            return; // deja present
        }
        if (global_args_2->threads_history[i] == 0)
        {
            global_args_2->threads_history[i] = thread_id;
            pthread_mutex_unlock(&acces_ids_history_2);
            return;
        }
        if (i == NB_THREAD_TOATL)
        {
            print("Erreur: nombre maximal de thread dans l'historique atteint, augmenter le #define NB_THREAD_TOATL");
            exit(1);
        }
    }
}

chemin solve_labyrinth_threads_1(Laby l, Case start, Case end)
{
    // si NB_THREAD == 1, lancer en recursif normal
    if (NB_THREAD <= 1)
    {
        printf("allouer plus de threads\n");
        exit(1);
    }
    if (cases_egales(start, CASE_NULLE) || cases_egales(end, CASE_NULLE))
    {
        printf("erreur dans les cases fournies a la fonction solve_labyrinth_threads\n");
        exit(1);
    }

    global_args_1 = malloc(sizeof(Thread_args));
    global_args_1->l = &l;
    global_args_1->end = end;
    global_args_1->res = malloc(NB_THREAD * sizeof(chemin));
    for (int i = 0; i < NB_THREAD; i++)
    {

        global_args_1->res[i] = malloc(CHEMIN_LENGTH * sizeof(chemin));

        for (int j = 1; j < CHEMIN_LENGTH; j++)
            global_args_1->res[i][j] = CASE_NULLE;

        global_args_1->res[i][0] = (Case){start.col, start.line};
    }
    global_args_1->threads = malloc(NB_THREAD * sizeof(pthread_t));
    for (int i = 0; i < NB_THREAD; i++)
        global_args_1->threads[i] = 0;
    global_args_1->threads_history = malloc(NB_THREAD_TOATL * sizeof(pthread_t));
    for (int i = 0; i < NB_THREAD_TOATL; i++)
        global_args_1->threads_history[i] = 0;
    chemin reponse_finale = malloc(CHEMIN_LENGTH * sizeof(Case));
    for (int i = 0; i < CHEMIN_LENGTH; i++)
        reponse_finale[i] = CASE_NULLE;

    pthread_mutex_lock(&solution_trouvee_1);

    pthread_create(&(global_args_1->threads[0]), NULL, (void *)rec_find_thread_1, NULL); // lancer la fonction dans un thread
    // directement enregister le thread dans lequel la fonction a ete lancee
    ajouter_dans_historique_1(global_args_1->threads[0]);

    pthread_mutex_lock(&solution_trouvee_1); // faire attendre le thread principale

    // join les threads (les chemins parcourus restent en memoire)
    for (int i = 0; i < NB_THREAD_TOATL; ++i)
        if (global_args_1->threads_history[i] > 0)
        {
            pthread_join(global_args_1->threads_history[i], NULL);
            global_args_1->threads_history[i] = -1;
        }

    // lire le chemin reponse dans le tableau de chemin :

    for (int i = 0; i < NB_THREAD; ++i)
    {
        if (cases_egales(global_args_1->res[i][getLastCaseIndex_1(i)], end))
        { // chemin directement trouvé
            for (int j = 0; j < CHEMIN_LENGTH; ++j)
                reponse_finale[j] = global_args_1->res[i][j];
            i = NB_THREAD; // stop boucle
        }
        else if (i == NB_THREAD)
            printf("Erreur: pas de chemin retenu\n");
    }

    // rendre l'espace utilisé
    for (int i = 0; i < NB_THREAD; ++i)
        free(global_args_1->res[i]);
    free(global_args_1->res);
    free(global_args_1->threads);
    free(global_args_1->threads_history);
    free(global_args_1);

    // remettre les cases depart & arrivee (peuvent etre malencontreusement remplacees par des cases VISITE lors de la recursivite)
    l.m[start.col][start.line] = ENTREE_1;
    l.m[end.col][end.line] = EXIT_1;

    return reponse_finale;
}

chemin solve_labyrinth_threads_2(Laby l, Case start, Case end)
{
    // si NB_THREAD == 1, lancer en recursif normal
    if (NB_THREAD <= 1)
    {
        printf("allouer plus de threads\n");
        exit(1);
    }
    if (cases_egales(start, CASE_NULLE) || cases_egales(end, CASE_NULLE))
    {
        printf("erreur dans les cases fournies a la fonction solve_labyrinth_threads\n");
        exit(1);
    }

    global_args_2 = malloc(sizeof(Thread_args));
    global_args_2->l = &l;
    global_args_2->end = end;
    global_args_2->res = malloc(NB_THREAD * sizeof(chemin));
    for (int i = 0; i < NB_THREAD; i++)
    {

        global_args_2->res[i] = malloc(CHEMIN_LENGTH * sizeof(chemin));

        for (int j = 1; j < CHEMIN_LENGTH; j++)
            global_args_2->res[i][j] = CASE_NULLE;

        global_args_2->res[i][0] = (Case){start.col, start.line};
    }
    global_args_2->threads = malloc(NB_THREAD * sizeof(pthread_t));
    for (int i = 0; i < NB_THREAD; i++)
        global_args_2->threads[i] = 0;
    global_args_2->threads_history = malloc(NB_THREAD_TOATL * sizeof(pthread_t));
    for (int i = 0; i < NB_THREAD_TOATL; i++)
        global_args_2->threads_history[i] = 0;
    chemin reponse_finale = malloc(CHEMIN_LENGTH * sizeof(Case));
    for (int i = 0; i < CHEMIN_LENGTH; i++)
        reponse_finale[i] = CASE_NULLE;

    pthread_mutex_lock(&solution_trouvee_2);

    pthread_create(&(global_args_2->threads[0]), NULL, (void *)rec_find_thread_2, NULL); // lancer la fonction dans un thread
    // directement enregister le thread dans lequel la fonction a ete lancee
    ajouter_dans_historique_2(global_args_2->threads[0]);

    pthread_mutex_lock(&solution_trouvee_2); // faire attendre le thread principale

    // join les threads (les chemins parcourus restent en memoire)
    for (int i = 0; i < NB_THREAD_TOATL; ++i)
        if (global_args_2->threads_history[i] > 0)
        {
            pthread_join(global_args_2->threads_history[i], NULL);
            global_args_2->threads_history[i] = -1;
        }

    // lire le chemin reponse dans le tableau de chemin :

    for (int i = 0; i < NB_THREAD; ++i)
    {
        if (cases_egales(global_args_2->res[i][getLastCaseIndex_2(i)], end))
        { // chemin directement trouvé
            for (int j = 0; j < CHEMIN_LENGTH; ++j)
                reponse_finale[j] = global_args_2->res[i][j];
            i = NB_THREAD; // stop boucle
        }
        else if (i == NB_THREAD)
            printf("Erreur: pas de chemin retenu\n");
    }

    // rendre l'espace utilisé
    for (int i = 0; i < NB_THREAD; ++i)
        free(global_args_2->res[i]);
    free(global_args_2->res);
    free(global_args_2->threads);
    free(global_args_2->threads_history);
    free(global_args_2);

    // remettre les cases depart & arrivee (peuvent etre malencontreusement remplacees par des cases VISITE lors de la recursivite)
    l.m[start.col][start.line] = ENTREE_1;
    l.m[end.col][end.line] = EXIT_1;

    return reponse_finale;
}

int get_thread_num_1()
{
    for (int i = 0; i < NB_THREAD; ++i)
        if (global_args_1->threads[i] == pthread_self())
            return i;
    return -1;
}

int get_first_room_for_new_thread_1()
{
    for (int i = 0; i < NB_THREAD; ++i)
        if (global_args_1->threads[i] == 0)
            return i;
    return -1;
}

int get_thread_num_2()
{
    for (int i = 0; i < NB_THREAD; ++i)
        if (global_args_2->threads[i] == pthread_self())
            return i;
    return -1;
}

int get_first_room_for_new_thread_2()
{
    for (int i = 0; i < NB_THREAD; ++i)
        if (global_args_2->threads[i] == 0)
            return i;
    return -1;
}


void copier_chemins_1(int from, int to){
    if(from < 0 || from >= NB_THREAD || to < 0 || to >= NB_THREAD){
        print("Erreur : Mauvais indice de chemins dans la fonction copier_chemins_1");
        exit(1);
    }if(to == from)
        return;
    for(int i = 0 ; i < CHEMIN_LENGTH; i++)
        if(cases_egales(global_args_1->res[to][i] = global_args_1->res[from][i], (Case){-1, -1}))
            break;
}

void copier_chemins_2(int from, int to){
    if(from < 0 || from >= NB_THREAD || to < 0 || to >= NB_THREAD){
        print("Erreur : Mauvais indice de chemins dans la fonction copier_chemins_2");
        exit(1);
    }if(to == from)
        return;
    for(int i = 0 ; i < CHEMIN_LENGTH; i++)
        if(cases_egales(global_args_2->res[to][i] = global_args_2->res[from][i], (Case){-1, -1}))
            break;
}

int une_possibilites_de_mouvement_1(Case c){
    if(cases_egales(c, CASE_NULLE))
        return 0;
    int temp; // utilisation de temp pour ne pas acceder 2 fois a la meme case de la matrice
    if(c.line-1 >= 0 && (temp = global_args_1->l->m[c.col][c.line-1]) != MUR && temp != VISITE_1 && temp != VISITE_12) return 1;
    if(c.col-1 >= 0 && (temp = global_args_1->l->m[c.col-1][c.line]) != MUR && temp != VISITE_1 && temp != VISITE_12) return 1;
    if(c.line+1 < global_args_1->l->cols && (temp = global_args_1->l->m[c.col][c.line+1]) != MUR && temp != VISITE_1 && temp != VISITE_12) return 1;
    if(c.col+1 < global_args_1->l->lignes && (temp = global_args_1->l->m[c.col+1][c.line]) != MUR && temp != VISITE_1 && temp != VISITE_12) return 1;
    return 0;
}

int une_possibilites_de_mouvement_2(Case c){
    if(cases_egales(c, CASE_NULLE))
        return 0;
    int temp; // utilisation de temp pour ne pas acceder 2 fois a la meme case de la matrice
    if(c.line-1 >= 0 && (temp = global_args_2->l->m[c.col][c.line-1]) != MUR && temp != VISITE_2 && temp != VISITE_12) return 1;
    if(c.col-1 >= 0 && (temp = global_args_2->l->m[c.col-1][c.line]) != MUR && temp != VISITE_2 && temp != VISITE_12) return 1;
    if(c.line+1 < global_args_2->l->cols && (temp = global_args_2->l->m[c.col][c.line+1]) != MUR && temp != VISITE_2 && temp != VISITE_12) return 1;
    if(c.col+1 < global_args_2->l->lignes && (temp = global_args_2->l->m[c.col+1][c.line]) != MUR && temp != VISITE_2 && temp != VISITE_12) return 1;
    return 0;
}


int est_dans_un_cul_de_sac_1(int t_id){
    if(t_id == -1){ // gestion d'erreur
        print("erreur, identifiant de thread == -1 dans la fonction est_dans_un_cul_de_sac_1");
        exit(1);
    }
    for(int i = 0 ;  i < CHEMIN_LENGTH && !cases_egales(global_args_1->res[t_id][i], CASE_NULLE) ; i++)
        if(une_possibilites_de_mouvement_1(global_args_1->res[t_id][i])) return 0; // pas de cul de sac
    //printf("%p\tcul de sac\n", (void*)pthread_self());
    //print_labyrinth(*global_args_1->l);
    return 1;
}

int est_dans_un_cul_de_sac_2(int t_id){
    if(t_id == -1){ // gestion d'erreur
        print("erreur, identifiant de thread == -1 dans la fonction est_dans_un_cul_de_sac_2");
        exit(1);
    }
    for(int i = 0 ;  i < CHEMIN_LENGTH && !cases_egales(global_args_2->res[t_id][i], CASE_NULLE) ; i++)
        if(une_possibilites_de_mouvement_2(global_args_2->res[t_id][i])) return 0; // pas de cul de sac
    //printf("%p\tcul de sac\n", (void*)pthread_self());
    //print_labyrinth(*global_args_2->l);
    return 1;
}

void rec_find_thread_1()
{
    // print_ids();
    //  ================ ARRET : solution trouvee =================
    if (pthread_mutex_trylock(&solution_trouvee_1) == 0)
    {
        pthread_mutex_unlock(&solution_trouvee_1);
        pthread_exit(NULL);
    }

    // ================ Recuperer le n° de thread, la case sur laquelle il se trouve et lrd caracteristiques de cette cas =================
    pthread_mutex_lock(&acces_ids_1);

    int thread_index = get_thread_num_1();
    int case_index = getLastCaseIndex_1(thread_index);
    if (case_index >= CHEMIN_LENGTH - 2)
    {
        print("pas assez de memoire allouee au vecteur de chemins");
        exit(1);
    }
    int ln = global_args_1->res[thread_index][case_index].line, cl = global_args_1->res[thread_index][case_index].col;

    pthread_mutex_unlock(&acces_ids_1);

    // ================ ARRET : je suis sur la case end ====================
    if (cases_egales((Case){cl, ln}, global_args_1->end))
    {
        pthread_mutex_lock(&acces_ids_1);
        pthread_mutex_unlock(&acces_ids_1);
        pthread_mutex_unlock(&solution_trouvee_1);
        pthread_exit(NULL);
    }
    if (ln - 1 >= 0)
    {
        pthread_mutex_lock(&acces_laby_1);
        if (global_args_1->l->m[cl][ln - 1] == WAY || global_args_1->l->m[cl][ln - 1] == EXIT_1)
        {
            global_args_1->l->m[cl][ln - 1] = VISITE_1;
            pthread_mutex_unlock(&acces_laby_1);
            pthread_mutex_lock(&acces_ids_1);
            if (!case_in_chemin(cl, ln - 1, global_args_1->res[thread_index]))
            {
                int indice_libre = get_first_room_for_new_thread_1();
                if (!une_possibilites_de_mouvement_1((Case){cl, ln}) || indice_libre == -1)
                {
                    ajouter_coord_et_nettoyer_apres(cl, ln - 1, global_args_1->res[thread_index]);
                    pthread_mutex_unlock(&acces_ids_1);
                    rec_find_thread_1();
                }
                else
                {
                    copier_chemins_1(thread_index, indice_libre);
                    ajouter_coord_et_nettoyer_apres(cl, ln - 1, global_args_1->res[indice_libre]);
                    pthread_create(&(global_args_1->threads[indice_libre]), NULL, (void *)rec_find_thread_1, NULL);
                    pthread_mutex_unlock(&acces_ids_1);
                    ajouter_dans_historique_1(global_args_1->threads[indice_libre]);
                }
            }
            else
                pthread_mutex_unlock(&acces_ids_1);
        }
        else
            pthread_mutex_unlock(&acces_laby_1);
    }
    if (cl - 1 >= 0)
    {
        pthread_mutex_lock(&acces_laby_1);
        if (global_args_1->l->m[cl - 1][ln] == WAY || global_args_1->l->m[cl - 1][ln] == EXIT_1)
        {
            global_args_1->l->m[cl - 1][ln] = VISITE_1;
            pthread_mutex_unlock(&acces_laby_1);
            pthread_mutex_lock(&acces_ids_1);
            if (!case_in_chemin(cl - 1, ln, global_args_1->res[thread_index]))
            {
                int indice_libre = get_first_room_for_new_thread_1();
                if (!une_possibilites_de_mouvement_1((Case){cl, ln}) || indice_libre == -1)
                {
                    ajouter_coord_et_nettoyer_apres(cl - 1, ln, global_args_1->res[thread_index]);
                    pthread_mutex_unlock(&acces_ids_1);
                    rec_find_thread_1();
                }
                else
                {
                    copier_chemins_1(thread_index, indice_libre);
                    ajouter_coord_et_nettoyer_apres(cl - 1, ln, global_args_1->res[indice_libre]);
                    pthread_create(&(global_args_1->threads[indice_libre]), NULL, (void *)rec_find_thread_1, NULL);
                    pthread_mutex_unlock(&acces_ids_1);
                    ajouter_dans_historique_1(global_args_1->threads[indice_libre]);
                }
            }
            else
                pthread_mutex_unlock(&acces_ids_1);
        }
        else
            pthread_mutex_unlock(&acces_laby_1);
    }
    if (ln + 1 < global_args_1->l->cols)
    {
        pthread_mutex_lock(&acces_laby_1);
        if (global_args_1->l->m[cl][ln + 1] == WAY || global_args_1->l->m[cl][ln + 1] == EXIT_1)
        {
            global_args_1->l->m[cl][ln + 1] = VISITE_1;
            pthread_mutex_unlock(&acces_laby_1);
            pthread_mutex_lock(&acces_ids_1);
            if (!case_in_chemin(cl, ln + 1, global_args_1->res[thread_index]))
            {
                int indice_libre = get_first_room_for_new_thread_1();
                if (!une_possibilites_de_mouvement_1((Case){cl, ln}) || indice_libre == -1)
                {
                    ajouter_coord_et_nettoyer_apres(cl, ln + 1, global_args_1->res[thread_index]);
                    pthread_mutex_unlock(&acces_ids_1);
                    rec_find_thread_1();
                }
                else
                {
                    copier_chemins_1(thread_index, indice_libre);
                    ajouter_coord_et_nettoyer_apres(cl, ln + 1, global_args_1->res[indice_libre]);
                    pthread_create(&(global_args_1->threads[indice_libre]), NULL, (void *)rec_find_thread_1, NULL);
                    pthread_mutex_unlock(&acces_ids_1);
                    ajouter_dans_historique_1(global_args_1->threads[indice_libre]);
                }
            }
            else
                pthread_mutex_unlock(&acces_ids_1);
        }
        else
            pthread_mutex_unlock(&acces_laby_1);
    }
    if (cl + 1 < global_args_1->l->lignes)
    {
        pthread_mutex_lock(&acces_laby_1);
        if (global_args_1->l->m[cl + 1][ln] == WAY || global_args_1->l->m[cl + 1][ln] == EXIT_1)
        {
            global_args_1->l->m[cl + 1][ln] = VISITE_1;
            pthread_mutex_unlock(&acces_laby_1);
            pthread_mutex_lock(&acces_ids_1);
            if (!case_in_chemin(cl + 1, ln, global_args_1->res[thread_index]))
            {
                int indice_libre = get_first_room_for_new_thread_1();
                if (!une_possibilites_de_mouvement_1((Case){cl, ln}) || indice_libre == -1)
                {
                    ajouter_coord_et_nettoyer_apres(cl + 1, ln, global_args_1->res[thread_index]);
                    pthread_mutex_unlock(&acces_ids_1);
                    rec_find_thread_1();
                }
                else
                {
                    copier_chemins_1(thread_index, indice_libre);
                    ajouter_coord_et_nettoyer_apres(cl + 1, ln, global_args_1->res[indice_libre]);
                    pthread_create(&(global_args_1->threads[indice_libre]), NULL, (void *)rec_find_thread_1, NULL);
                    pthread_mutex_unlock(&acces_ids_1);
                    ajouter_dans_historique_1(global_args_1->threads[indice_libre]);
                }
            }
            else
                pthread_mutex_unlock(&acces_ids_1);
        }
        else
            pthread_mutex_unlock(&acces_laby_1);
    }
    if (est_dans_un_cul_de_sac_1(thread_index))
    {
        pthread_mutex_lock(&acces_ids_1);
        for (int i = 0; i <= case_index + 1; ++i)
            global_args_1->res[thread_index][i] = CASE_NULLE;
        global_args_1->threads[thread_index] = 0;
        pthread_mutex_unlock(&acces_ids_1);
        pthread_exit(NULL);
    }
}

void rec_find_thread_2()
{
    // print_ids();
    //  ================ ARRET : solution trouvee =================
    if (pthread_mutex_trylock(&solution_trouvee_2) == 0)
    {
        pthread_mutex_unlock(&solution_trouvee_2);
        pthread_exit(NULL);
    }

    // ================ Recuperer le n° de thread, la case sur laquelle il se trouve et lrd caracteristiques de cette cas =================
    pthread_mutex_lock(&acces_ids_2);

    int thread_index = get_thread_num_2();
    int case_index = getLastCaseIndex_2(thread_index);
    if (case_index >= CHEMIN_LENGTH - 2)
    {
        print("pas assez de memoire allouee au vecteur de chemins");
        exit(1);
    }
    int ln = global_args_2->res[thread_index][case_index].line, cl = global_args_2->res[thread_index][case_index].col;

    pthread_mutex_unlock(&acces_ids_2);

    // ================ ARRET : je suis sur la case end ====================
    if (cases_egales((Case){cl, ln}, global_args_2->end))
    {
        pthread_mutex_lock(&acces_ids_2);
        pthread_mutex_unlock(&acces_ids_2);
        pthread_mutex_unlock(&solution_trouvee_2);
        pthread_exit(NULL);
    }
    if (ln - 1 >= 0)
    {
        pthread_mutex_lock(&acces_laby_2);
        if (global_args_2->l->m[cl][ln - 1] == WAY || global_args_2->l->m[cl][ln - 1] == EXIT_2)
        {
            global_args_2->l->m[cl][ln - 1] = VISITE_2;
            pthread_mutex_unlock(&acces_laby_2);
            pthread_mutex_lock(&acces_ids_2);
            if (!case_in_chemin(cl, ln - 1, global_args_2->res[thread_index]))
            {
                int indice_libre = get_first_room_for_new_thread_2();
                if (!une_possibilites_de_mouvement_2((Case){cl, ln}) || indice_libre == -1)
                {
                    ajouter_coord_et_nettoyer_apres(cl, ln - 1, global_args_2->res[thread_index]);
                    pthread_mutex_unlock(&acces_ids_2);
                    rec_find_thread_2();
                }
                else
                {
                    copier_chemins_2(thread_index, indice_libre);
                    ajouter_coord_et_nettoyer_apres(cl, ln - 1, global_args_2->res[indice_libre]);
                    pthread_create(&(global_args_2->threads[indice_libre]), NULL, (void *)rec_find_thread_2, NULL);
                    pthread_mutex_unlock(&acces_ids_2);
                    ajouter_dans_historique_2(global_args_2->threads[indice_libre]);
                }
            }
            else
                pthread_mutex_unlock(&acces_ids_2);
        }
        else
            pthread_mutex_unlock(&acces_laby_2);
    }
    if (cl - 1 >= 0)
    {
        pthread_mutex_lock(&acces_laby_2);
        if (global_args_2->l->m[cl - 1][ln] == WAY || global_args_2->l->m[cl - 1][ln] == EXIT_2)
        {
            global_args_2->l->m[cl - 1][ln] = VISITE_2;
            pthread_mutex_unlock(&acces_laby_2);
            pthread_mutex_lock(&acces_ids_2);
            if (!case_in_chemin(cl - 1, ln, global_args_2->res[thread_index]))
            {
                int indice_libre = get_first_room_for_new_thread_2();
                if (!une_possibilites_de_mouvement_2((Case){cl, ln}) || indice_libre == -1)
                {
                    ajouter_coord_et_nettoyer_apres(cl - 1, ln, global_args_2->res[thread_index]);
                    pthread_mutex_unlock(&acces_ids_2);
                    rec_find_thread_2();
                }
                else
                {
                    copier_chemins_2(thread_index, indice_libre);
                    ajouter_coord_et_nettoyer_apres(cl - 1, ln, global_args_2->res[indice_libre]);
                    pthread_create(&(global_args_2->threads[indice_libre]), NULL, (void *)rec_find_thread_2, NULL);
                    pthread_mutex_unlock(&acces_ids_2);
                    ajouter_dans_historique_2(global_args_2->threads[indice_libre]);
                }
            }
            else
                pthread_mutex_unlock(&acces_ids_2);
        }
        else
            pthread_mutex_unlock(&acces_laby_2);
    }
    if (ln + 1 < global_args_2->l->cols)
    {
        pthread_mutex_lock(&acces_laby_2);
        if (global_args_2->l->m[cl][ln + 1] == WAY || global_args_2->l->m[cl][ln + 1] == EXIT_2)
        {
            global_args_2->l->m[cl][ln + 1] = VISITE_2;
            pthread_mutex_unlock(&acces_laby_2);
            pthread_mutex_lock(&acces_ids_2);
            if (!case_in_chemin(cl, ln + 1, global_args_2->res[thread_index]))
            {
                int indice_libre = get_first_room_for_new_thread_2();
                if (!une_possibilites_de_mouvement_2((Case){cl, ln}) || indice_libre == -1)
                {
                    ajouter_coord_et_nettoyer_apres(cl, ln + 1, global_args_2->res[thread_index]);
                    pthread_mutex_unlock(&acces_ids_2);
                    rec_find_thread_2();
                }
                else
                {
                    copier_chemins_2(thread_index, indice_libre);
                    ajouter_coord_et_nettoyer_apres(cl, ln + 1, global_args_2->res[indice_libre]);
                    pthread_create(&(global_args_2->threads[indice_libre]), NULL, (void *)rec_find_thread_2, NULL);
                    pthread_mutex_unlock(&acces_ids_2);
                    ajouter_dans_historique_2(global_args_2->threads[indice_libre]);
                }
            }
            else
                pthread_mutex_unlock(&acces_ids_2);
        }
        else
            pthread_mutex_unlock(&acces_laby_2);
    }
    if (cl + 1 < global_args_2->l->lignes)
    {
        pthread_mutex_lock(&acces_laby_2);
        if (global_args_2->l->m[cl + 1][ln] == WAY || global_args_2->l->m[cl + 1][ln] == EXIT_2)
        {
            global_args_2->l->m[cl + 1][ln] = VISITE_2;
            pthread_mutex_unlock(&acces_laby_2);
            pthread_mutex_lock(&acces_ids_2);
            if (!case_in_chemin(cl + 1, ln, global_args_2->res[thread_index]))
            {
                int indice_libre = get_first_room_for_new_thread_2();
                if (!une_possibilites_de_mouvement_2((Case){cl, ln}) || indice_libre == -1)
                {
                    ajouter_coord_et_nettoyer_apres(cl + 1, ln, global_args_2->res[thread_index]);
                    pthread_mutex_unlock(&acces_ids_2);
                    rec_find_thread_2();
                }
                else
                {
                    copier_chemins_2(thread_index, indice_libre);
                    ajouter_coord_et_nettoyer_apres(cl + 1, ln, global_args_2->res[indice_libre]);
                    pthread_create(&(global_args_2->threads[indice_libre]), NULL, (void *)rec_find_thread_2, NULL);
                    pthread_mutex_unlock(&acces_ids_2);
                    ajouter_dans_historique_2(global_args_2->threads[indice_libre]);
                }
            }
            else
                pthread_mutex_unlock(&acces_ids_2);
        }
        else
            pthread_mutex_unlock(&acces_laby_2);
    }
    if (est_dans_un_cul_de_sac_2(thread_index))
    {
        pthread_mutex_lock(&acces_ids_2);
        for (int i = 0; i <= case_index + 1; ++i)
            global_args_2->res[thread_index][i] = CASE_NULLE;
        global_args_2->threads[thread_index] = 0;
        pthread_mutex_unlock(&acces_ids_2);
        pthread_exit(NULL);
    }
}

void print(const char *msg)
{
    pthread_mutex_lock(&acces_out);
    printf("%p:\t%s\n", (void *)pthread_self(), msg);
    pthread_mutex_unlock(&acces_out);
}

void print_solution(Laby l, chemin c)
{
    // ajouter des lettres dans le labyrinth pour montrer le chemin pris
    int ch = 'a';
    for (int i = 1; i < CHEMIN_LENGTH - 1; ++i)
    {
        if (cases_egales(c[i + 1], CASE_NULLE))
            break;
        else
            l.m[c[i].col][c[i].line] = ch++; //'@'; // affichage du chemin avec les characteres ascii a,b->z,A,B->Z
        if (ch == 'z' + 1)
            ch = 'A';
        if (ch == 'Z' + 1)
            ch = 'a';
    }

    print_labyrinth(l); // afficher le labyrinth avec les modifications faites (qui seront prises en comptes par le default du switch)

    // retirer les lettres ensuite
    for (int i = 1; i < CHEMIN_LENGTH - 1; ++i)
    {
        if (cases_egales(c[i + 1], CASE_NULLE))
            break;
        else
            l.m[c[i].col][c[i].line] = WAY;
    }
}

void print_chemin(chemin c)
{
    printf("[");
    for (int i = 0; i < CHEMIN_LENGTH; i++)
    {
        if (cases_egales(c[i], CASE_NULLE))
            break;
        printf(" %d;%d ", c[i].col, c[i].line);
    }
    printf("]\n");
}

int ajouter_coord_et_nettoyer_apres(int col, int line, chemin c)
{
    if (col == UNUSED || line == UNUSED)
    {
        print("erreur dans le format de la case");
        return 0;
    }
    Case s = {col, line};
    if (cases_egales(c[0], CASE_NULLE))
    {
        c[0] = s;
        for (int j = 1; !cases_egales(c[j], CASE_NULLE) && j < CHEMIN_LENGTH; ++j)
            c[j] = CASE_NULLE;
        return 1;
    }
    for (int i = CHEMIN_LENGTH - 2; i >= 0; --i)
        if (!cases_egales(c[i], CASE_NULLE))
            if (sont_voisines(c[i], s))
            {                 // skipper toutes les cases de coordonnees{-1 ; -1} --optimisation du if possible
                c[i + 1] = s; // case ajoutee
                for (int j = i + 2; !cases_egales(c[j], CASE_NULLE) && j < CHEMIN_LENGTH; ++j)
                    c[j] = CASE_NULLE;
                return 1;
            }
    printf("echec d'ajout de la case {%d, %d}", col, line);
    print("");
    // print_ids();
    return 0; // erreur aucune case ajoutee
}

int case_in_chemin(int col, int line, chemin c)
{
    int a, b; // utiliser des variables temporaires pour plus d'efficacité ?
    for (int i = 0; i < CHEMIN_LENGTH; ++i)
    {
        if ((a = c[i].col) == UNUSED || (b = c[i].line) == UNUSED)
            return 0;
        if (a == col && b == line)
            return 1;
    }
    return 0;
}

Laby creer_labyrinth(int cols, int lines)
{
    if (cols < 4)
    {
        printf("erreur : il faut que cols soit superieur ou egal à 5, vous avez passe %d\n", cols);
        exit(1);
    }
    if (lines < 4)
    {
        printf("erreur : il faut que lines soit superieur ou egal à 5, vous avez passe %d\n", lines);
        exit(1);
    }
    char *command = malloc(128);
    sprintf(command, "python3 generateur.py %d %d > out.txt", lines, cols);
    system(command);
    free(command);
    FILE *out = fopen("out.txt", "r");
    if (out == NULL)
    {
        printf("Failed to open file\n");
        exit(1);
    }

    Laby current_laby;
    current_laby.m = malloc(lines * sizeof(int *));
    for (int i = 0; i < lines; i++)
        current_laby.m[i] = (int *)malloc(cols * sizeof(int));

    int c;

    for (int i = 0; i < lines; i++)
    {
        for (int j = 0; j < cols; j++)
            if ((c = fgetc(out)) != '\n')
                current_laby.m[i][j] = c - '0';
        fgetc(out); // ne pas prendre en compte le caracter "newline"
    }

    // placer la deuxieme entree, la deuxime sortie et la porte :
    srand(time(NULL));
    int ii, jj;
    do
    {
        ii = rand() % lines;
        jj = rand() % cols;
    } while (current_laby.m[ii][jj] != WAY);
    current_laby.m[ii][jj] = ENTREE_2;

    do
    {
        ii = rand() % lines;
        jj = rand() % cols;
    } while (current_laby.m[ii][jj] != WAY);
    current_laby.m[ii][jj] = PORTE;

    do
    {
        ii = rand() % lines;
        jj = rand() % cols;
    } while (current_laby.m[ii][jj] != WAY);
    current_laby.m[ii][jj] = EXIT_2;

    fclose(out);
    current_laby.lignes = lines;
    current_laby.cols = cols;
    return current_laby;
}

int abso(int x) { return x < 0 ? -x : x; }
int sont_voisines(Case a, Case b) { return (a.col == b.col && abso(a.line - b.line) == 1) || (a.line == b.line && abso(a.col - b.col) == 1); }
int cases_egales(Case a, Case b) { return a.col == b.col && a.line == b.line; }
void print_Case(Case s) { printf("%d %d\n", s.col, s.line); }

Case trouver_entree_1(Laby l)
{
    // trouvons le depart
    Case start = CASE_NULLE;
    for (int i = 0; i < l.lignes; i++)
        for (int j = 0; j < l.cols; j++)
            if (l.m[i][j] == ENTREE_1)
            {
                start.col = i;
                start.line = j;
                // arret des boucles
                i = l.lignes;
                j = l.cols;
            }

    if (cases_egales(start, CASE_NULLE))
    {
        printf("Impossible de trouver l'entree\n");
        exit(1);
    }
    return start;
}

Case trouver_sortie_1(Laby l)
{
    // trouvons l'arivee
    Case end = CASE_NULLE;
    for (int i = 0; i < l.lignes; i++)
        for (int j = 0; j < l.cols; j++)
            if (l.m[i][j] == EXIT_1)
            {
                end.col = i;
                end.line = j;
                // arret des boucles
                i = l.lignes;
                j = l.cols;
            }

    if (cases_egales(end, CASE_NULLE))
    {
        printf("Impossible de trouver la sortie\n");
        exit(1);
    }
    return end;
}

Case trouver_entree_2(Laby l)
{
    // trouvons le depart
    Case start = CASE_NULLE;
    for (int i = 0; i < l.lignes; i++)
        for (int j = 0; j < l.cols; j++)
            if (l.m[i][j] == ENTREE_2)
            {
                start.col = i;
                start.line = j;
                // arret des boucles
                i = l.lignes;
                j = l.cols;
            }

    if (cases_egales(start, CASE_NULLE))
    {
        printf("Impossible de trouver l'entree\n");
        exit(1);
    }
    return start;
}

Case trouver_sortie_2(Laby l)
{
    // trouvons l'arivee
    Case end = CASE_NULLE;
    for (int i = 0; i < l.lignes; i++)
        for (int j = 0; j < l.cols; j++)
            if (l.m[i][j] == EXIT_2)
            {
                end.col = i;
                end.line = j;
                // arret des boucles
                i = l.lignes;
                j = l.cols;
            }

    if (cases_egales(end, CASE_NULLE))
    {
        printf("Impossible de trouver la sortie\n");
        exit(1);
    }
    return end;
}

Case trouver_porte(Laby l)
{
    // trouvons le depart
    Case start = CASE_NULLE;
    for (int i = 0; i < l.lignes; i++)
        for (int j = 0; j < l.cols; j++)
            if (l.m[i][j] == PORTE)
            {
                start.col = i;
                start.line = j;
                // arret des boucles
                i = l.lignes;
                j = l.cols;
            }

    if (cases_egales(start, CASE_NULLE))
    {
        printf("Impossible de trouver l'entree\n");
        exit(1);
    }
    return start;
}

void free_labyrinth(Laby *l)
{
    if (l->m == NULL)
        return; // rien a faire

    for (int i = 0; i < l->lignes; i++)
        if (l->m[i] != NULL)
            free(l->m[i]);

    if (l->m != NULL)
        free(l->m);

    l->m = NULL;
}

int check_solution(Case *Case_tab, Case start, Case end)
{
    // verifier ENTREE
    if (!cases_egales(Case_tab[0], start))
        return 0; // false

    int index_end;
    // avancer index_end jusqu'au cases inutilisees
    for (index_end = 0; index_end < CHEMIN_LENGTH - 2; index_end++)
        if (cases_egales(Case_tab[index_end + 1], CASE_NULLE))
            break;
    if (!cases_egales(Case_tab[index_end], start))
        return 0; // false

    if (index_end == CHEMIN_LENGTH - 1)
        return 0; // pas de fin de chemin

    // verifier que les cases sont voisines
    for (int i = 1; i <= index_end; i++)
        if (!sont_voisines(Case_tab[i], Case_tab[i - 1]))
            return 0;

    return 1; // tous tests passes
}

void print_labyrinth(Laby l)
{
    for (int i = 0; i < l.cols; i++)
    {
        if (i < 10)
            printf(" ");
        else
            printf("%d", i / 10);
    }
    printf("|\n");
    for (int i = 0; i < l.cols; i++)
        printf("%d", i % 10);
    printf("|\n");
    for (int i = 0; i < l.cols; i++)
        printf("-");

    printf("|+\n");
    for (int i = 0; i < l.lignes; i++)
    {
        for (int j = 0; j < l.cols; j++)
            switch (l.m[i][j])
            {
            case MUR:
                printf("#");
                break;
            case WAY:
                printf(" ");
                break;
            case ENTREE_1:
                printf("1");
                break;
            case ENTREE_2:
                printf("2");
                break;
            case EXIT_1:
                printf("1");
                break;
            case EXIT_2:
                printf("2");
                break;
            case VISITE_1:
                printf(" ");
                break;
            case VISITE_2:
                printf(" ");
                break;
            case PORTE:
                printf("P");
                break;
            default:
                printf("%c", l.m[i][j]);
                break;
            }
        printf("|%d\n", i);
    }
}

void print_raw_labyrinth(Laby l)
{
    // print upper indexs :
    for (int i = 0; i < l.cols; i++)
        printf("%d\t\t", i % 10);
    printf("|+\n");
    for (int i = 0; i < l.lignes; i++)
    {
        for (int j = 0; j < l.cols; j++)
            printf("m[%d, %d]=%d\t", i, j, l.m[i][j]);
        printf("|%d\n", i);
    }
}