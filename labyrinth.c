#include "labyrinth.h"

Thread_manager creer_threads(){
    Thread_manager res;
    res.used = malloc(NB_THREAD*sizeof(int));
    res.ids = malloc(NB_THREAD*sizeof(pthread_t));
    res.sons = malloc(NB_THREAD*sizeof(pthread_t*));
    for(int i = 0 ; i < NB_THREAD ; i++)
        res.sons[i] = malloc(NB_THREAD*sizeof(pthread_t));
    // initialisation
    for(int i = 0 ; i < NB_THREAD ; i++){
        res.used[i] = res.ids[i] = 0;
        for(int j = 0 ; j  < NB_THREAD ;++j)
            res.sons[i][j] = 0;
    }
    return res;
}

void free_threads(Thread_manager *t){
    if(t == NULL)
        return;
    if(t->used != NULL)
        free(t->used);
    if(t->ids != NULL)
        free(t->ids);
    if(t->sons != NULL)
        for(int i = 0 ; i < NB_THREAD ; ++i)
            if(t->sons[i] != NULL)
                free(t->sons[i]);
    t = NULL;
}

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

    chemin reponse = malloc(sizeof(chemin) * CHEMIN_LENGTH);
    
    for(int i = 0; i < CHEMIN_LENGTH; i++)
        reponse[i] = (Case){UNUSED, UNUSED};

    // preparer de l'espace pour les threads
    Thread_manager tm = creer_threads();
    int sol = 0;
    // lancer la recursivite
    Thread_args args = {&l, reponse, &start, &end, &tm, pthread_self(), &sol};
    printf("avant le lancement de la recursivite\n");
    rec_find_thread((void*)&args);
    printf("apres le lancement de la recursivite\n");
    // rendre l'espace utilisé
    free_threads(&tm);
    
    //remettre la case depart : 
    l.m[start.col][start.line] = ENTREE;

    // nettoyer la reponse si necessaire : 
    if(!check_solution(l, reponse)) nettoyer_chemin(reponse);

    return reponse;
}
void end_actual_thread_signal_without_cancel(Thread_manager *t){
    for(int i = 0 ; i  < NB_THREAD ; i++)
        if(pthread_self() == t->ids[i]){
            t->used[i] = 0;
            return;
        }
}

void end_actual_thread_signal(Thread_manager *t){
    for(int i = 0 ; i  < NB_THREAD ; i++)
        if(pthread_self() == t->ids[i]){
            t->used[i] = 0;
            return;
        }
    pthread_cancel(pthread_self());
}

int actual_ind(Thread_manager *t){
    for(int i = 0 ; i < NB_THREAD ; i++)
        if(pthread_self() == t->ids[i])
            return i;
    return -1;
}

int nb_sons(Thread_manager *t){
    int count = 0;
    for(int i = 0 ; i < NB_THREAD ; i++)
        count += (t->sons[i] != 0);
    return count;
}

void end_actual_thread_and_sons_return_best_chemin(Thread_manager *t){
    chemin rep = malloc(NB_THREAD*sizeof(Case)); // ATTENTION ne pas oublier le 'FREE' apres
    Case temp[NB_THREAD];
    temp[0] = (Case){UNUSED, UNUSED};
    rep[0] = (Case){UNUSED, UNUSED};
    int ret;
    for(int i = 0 ; i < nb_sons(t) ; ++i){ // pour chaque fils
        ret = pthread_join(t->sons[i], (void**)&temp);
        if(ret > 0){
            printf("erreur fermeture du thread %ld\tcode%d\n", t->sons[i], ret);
            exit(1);
        }
        if(rep[0].col == UNUSED && rep[0].line == UNUSED && temp[0].col != UNUSED && temp[0].line != UNUSED) // copy temp in rep if rep isn't an answer and temp is an answer
            for(int i = 0 ; i  < NB_THREAD ; i++)
                rep[i] = temp[i]; 
    }
    pthread_exit((void*)rep);
}


void end_actual_thread_and_sons_return_self_chemin(Thread_args *ta){
    int ret;
    for(int i = 0 ; i < nb_sons(ta->tm) ; ++i){ // pour chaque fils
        ret = pthread_join(ta->tm->sons[i], NULL);
        if(ret > 0){
            printf("erreur fermeture du thread %ld\tcode%d\n", ta->tm->sons[i], ret);
            exit(1);
        }
    }
    pthread_exit((void*)ta->res);
}



// gerer le cas du pere qui doit retourner le chemin finale
chemin rec_find_thread(void* th_args){
    Thread_args* t = (Thread_args*)th_args;
    
    // si une réponse est trouvée, finir le thread actuel, seulement s'il n'est père d'aucun thread, sinon attendre ses fils
    if(*(t->solution_trouvee) && pthread_self() != t->father)
        end_actual_thread_and_sons_return_best_chemin(t->tm);

    //verifier si la case end est atteinte
    if(cases_egales(*t->current, *t->end)){
         // ajouter à la main la derniere case dans le chemin :
        for(int i=0; i<CHEMIN_LENGTH; i++)
            if(t->res[i].col == UNUSED && t->res[i].line == UNUSED){
                t->res[i] = *t->end;
                break;
            }
        t->res[CHEMIN_LENGTH-1] = (Case){END_SIGNAL, END_SIGNAL};
        *t->solution_trouvee = 1;
        if(pthread_self() == t->father)
            return t->res; // retourner le resultat normalement si processus pere, celui qui a ete appele par la fonction initialisant la recusivite
        else    
            end_actual_thread_and_sons_return_self_chemin(t); // arreter les processus fils et renvoyer au pere la solution
    }



    // marquer la case comme visitée : 
    t->l->m[t->current->col][t->current->line] = VISITE;

    //ajouter la case dans le chemin
    ajouter_coordonees_au_chemin_au_dernier_voisin(t->current->col, t->current->line, t->res);

    // verifier les 4 directions // si un choix, lancer une recursivite simple, si plusieurs choix, lancer une recursivite et completer avec des threads
    // chaque thread doit etre lance avec une copie du chemin pour eviter quils ecrivent tous dans le meme vecteur
    int up = 0, left = 0, right = 0, down = 0;
    if(t->current->line-1 >= 0 && !Case_in_chemin(t->current->col, t->current->line-1, t->res) && t->l->m[t->current->col][t->current->line-1] != MUR && t->l->m[t->current->col][t->current->line-1] !=  VISITE) up = 1;
    if(t->current->col - 1 >= 0 && !Case_in_chemin(t->current->col-1, t->current->line, t->res) && t->l->m[t->current->col-1][t->current->line] != MUR && t->l->m[t->current->col-1][t->current->line] !=  VISITE) left = 1;  
    if(t->current->line+1 < t->l->cols && !Case_in_chemin(t->current->col, t->current->line+1, t->res) && t->l->m[t->current->col][t->current->line+1] != MUR && t->l->m[t->current->col][t->current->line+1] !=  VISITE) down = 1;
    if(t->current->col+1 < t->l->lignes && !Case_in_chemin(t->current->col+1, t->current->line, t->res) && t->l->m[t->current->col+1][t->current->line] != MUR && t->l->m[t->current->col+1][t->current->line] !=  VISITE) right = 1;

    int nb_ways = 0;
    if(up){
        // simple recusivite
        Thread_args nt;
        Case ncc = {t->current->col, t->current->line-1};
        nt.current = &ncc;
        nt.end = t->end;
        nt.father = t->father;
        nt.l = t->l;
        nt.solution_trouvee = t->solution_trouvee;
        
        nt.tm = t->tm;
        rec_find_thread((void*)&nt);

        ++nb_ways;
    }


    if(left){
        if(nb_ways == 0){ // simple recursivite
            Thread_args nt;
            Case ncc = {t->current->col, t->current->line-1};
            nt.current = &ncc;
            nt.end = t->end;
            nt.father = t->father;
            nt.l = t->l;
            nt.solution_trouvee = t->solution_trouvee;

            nt.tm = t->tm;
            rec_find_thread((void*)&nt);
        }else{ // creer un thread

            //

        }
        ++nb_ways;  
    }




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
        if(!(c[i].col == UNUSED && c[i].line == UNUSED) && sont_voisines(c[i], a_ajouter)){// skipper toutes les cases à la fin de coordonnees{-1 ; 1}
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