#include "labyrinth.h"

Thread_manager creer_threads(){
    Thread_manager res;
    res.used = malloc(NB_THREAD*sizeof(int));
    res.ids = malloc(NB_THREAD*sizeof(pthread_t));
    return res;
}

void free_threads(Thread_manager *t){
    if(t == NULL)
        return;
    if(t->used != NULL)
        free(t->used);
    if(t->ids != NULL)
        free(t->ids);
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
    // lancer la recursivite
    Thread_args* args = {&l, reponse, &start, &end, &tm};
    rec_find_thread((void*)args);
    // rendre l'espace utilisé
    free_threads(&tm);
    
    //remettre la case depart : 
    l.m[start.col][start.line] = ENTREE;

    // nettoyer la reponse si necessaire : 
    if(!check_solution(l, reponse)) nettoyer_chemin(reponse);

    return reponse;
}

void end_actual_thread_signal(Thread_manager *t){
    for(int i = 0 ; i  < NB_THREAD ; i++)
        if(pthread_self() == t->ids[i]){
            t->used[i] = 0;
            return;
        }
    pthread_cancel(pthread_self());
}

void rec_find_thread(void* th_args){
    Thread_args* t = (Thread_args*)th_args;
    chemin _res = t->res;
    Laby* _l = t->l;
    Case* _current = t->current;
    Case* _end = t->end;
    Thread_manager* _manager = t->tm;

    if(_res[CHEMIN_LENGTH-1].col == END_SIGNAL && _res[CHEMIN_LENGTH-1].line == END_SIGNAL) // check end_signal
        end_actual_thread_signal(_manager); // une solution a deja ete trouvee
    
    if(cases_egales(*_current, *_end)){

        // ajouter à la main la derniere case dans le chemin :
        for(int i=0; i<CHEMIN_LENGTH; i++)
            if(_res[i].col == UNUSED && _res[i].line == UNUSED){
                _res[i] = (*_end);
                break;
            }
        _res[CHEMIN_LENGTH-1] = (Case){END_SIGNAL, END_SIGNAL};
        end_actual_thread_signal(_manager); // une solution a deja ete trouvee
    }

    // marquer la case comme visitée : 
    _l->m[_current->col][_current->line] = VISITE;

    //ajouter la case dans le chemin
    ajouter_coordonees_au_chemin_au_dernier_voisin(_current->col, _current->line, _res);
    pthread_t threads_crees[4];

    for(int i = 0; i < 4 ; i++) threads_crees[i] = 0;
    // verifier les 4 directions et lancer une récursivité avec un thread si possible (s'il en reste) ou sinon lancer une recursitivté simple
    if(_current->line-1 >= 0 && !Case_in_chemin(_current->col, _current->line-1, _res) && _l->m[_current->col][_current->line-1] != MUR && _l->m[_current->col][_current->line-1] !=  VISITE){ // left
        // vérifier qu'il reste un thread dispo
        Thread_args* ta = {_l, _res, (Case*){_current->col, _current->line-1},_end, (void*)_manager};
        for(int i = 0 ; i < NB_THREAD; i++)
            if(_manager->used[i] == 0){ // disponible
                _manager->used[i] = 1; // marquer comme utilisé
                pthread_create(_manager->ids + i, NULL, (void*)rec_find_thread, (void*)ta);
                threads_crees[0] = _manager->ids[i];
            }
        // si non cree, lancer la reucrisvité sans thread:
        if(threads_crees[0] == 0)
            rec_find_thread((void*)ta); 
    }
    if(_current->col - 1 >= 0 && !Case_in_chemin(_current->col-1, _current->line, _res) && _l->m[_current->col-1][_current->line] != MUR && _l->m[_current->col-1][_current->line] !=  VISITE){ // up
        // vérifier qu'il reste un thread dispo
        Thread_args*ta = {_l, _res, (Case*){_current->col-1, _current->line},_end, (void*)_manager};
        for(int i = 0 ; i < NB_THREAD; i++)
            if(_manager->used[i] == 0){ // disponible
                _manager->used[i] = 1; // marquer comme utilisé
                pthread_create(_manager->ids + i, NULL, (void*)rec_find_thread, (void*)ta);
                threads_crees[1] = _manager->ids[i];
            }
        // si non cree, lancer la reucrisvité sans thread:
        if(threads_crees[1] == 0)
            rec_find_thread((void*)ta); 
    }
    if(_current->line+1 < _l->cols && !Case_in_chemin(_current->col, _current->line+1, _res) && _l->m[_current->col][_current->line+1] != MUR && _l->m[_current->col][_current->line+1] !=  VISITE){ // right
        // vérifier qu'il reste un thread dispo
        Thread_args*ta = {_l, _res, (Case*){_current->col, _current->line+1},_end, (void*)_manager};
        for(int i = 0 ; i < NB_THREAD; i++)
            if(_manager->used[i] == 0){ // disponible
                _manager->used[i] = 1; // marquer comme utilisé
                pthread_create(_manager->ids + i, NULL, (void*)rec_find_thread, (void*)ta);
                threads_crees[2] = _manager->ids[i];
            }
        // si non cree, lancer la reucrisvité sans thread:
        if(threads_crees[2] == 0)
            rec_find_thread((void*)ta); 
    }
    if(_current->col+1 < _l->lignes && !Case_in_chemin(_current->col+1, _current->line, _res) && _l->m[_current->col+1][_current->line] != MUR && _l->m[_current->col+1][_current->line] !=  VISITE){ // down
        // vérifier qu'il reste un thread dispo
        Thread_args*ta = {_l, _res, (Case*){_current->col+1, _current->line},_end, (void*)_manager};
        for(int i = 0 ; i < NB_THREAD; i++)
            if(_manager->used[i] == 0){ // disponible
                _manager->used[i] = 1; // marquer comme utilisé
                pthread_create(_manager->ids + i, NULL, (void*)rec_find_thread, (void*)ta);
                threads_crees[3] = _manager->ids[i];
            }
        // si non cree, lancer la reucrisvité sans thread:
        if(threads_crees[3] == 0)
            rec_find_thread((void*)ta); 
    }

    // attendre que les potentiels threads crees se finissent

    for(int i = 0 ; i < 4 ; ++i)
        if(threads_crees[i] != 0)
            pthread_join(threads_crees[i], NULL);


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
    if(current.line+1 < l.cols && !Case_in_chemin(current.col, current.line+1, res) && l.m[current.col][current.line+1] != MUR && l.m[current.col][current.line+1] !=  VISITE) // right
        rec_find(l, res, (Case){current.col, current.line+1}, end);
    if(current.col+1 < l.lignes && !Case_in_chemin(current.col+1, current.line, res) && l.m[current.col+1][current.line] != MUR && l.m[current.col+1][current.line] !=  VISITE) // down
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