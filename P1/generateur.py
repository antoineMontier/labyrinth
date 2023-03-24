import click
import random

def pos_voisins(pos, lignes, colonnes):
    x, y = pos
    res = []
    for i in range(-1, 2, 2):
        if x + i > 0 and x + i < lignes - 1:
            res.append([x + i, y])
    for j in range(-1, 2, 2):
        if y + j > 0 and y + j < colonnes - 1:
            res.append([x, y + j])
    return res

def combien_visite(visites, pos):
    res = 0
    for voisin in pos_voisins(pos, len(visites), len(visites[0])):
        x, y = voisin
        res += 1 if visites[x][y] else 0
    return res

def ajouter_murs_unique(murs, pos, lab):
    x, y = pos
    if pos not in murs and lab[x][y] == 0:
        murs.append(pos)

def ligne_sortie(lab):
    return random.choice([i for i, ligne in enumerate(lab) if ligne[-2] == 1])

def voisin_vide(lab, pos):
    for voisin in pos_voisins(pos, len(lab), len(lab[0])):
        x, y = voisin
        if lab[x][y] == 1:
            return voisin
        
def diff_vecteur(u, v):
    x1, y1 = u
    x2, y2 = v
    return [x1 - x2, y1 - y2]

def somme_vecteur(u, v):
    x1, y1 = u
    x2, y2 = v
    return [x1 + x2, y1 + y2]    

def neg_vecteur(u):
    return [-u[0], -u[1]]

def printLab(lab):
    for ligne in lab:
        print("".join(map(str, ligne)))


def generation(colonnes, lignes):
    res    = [[0] * colonnes for i in range(lignes)]
    enter  = random.randint(1, lignes - 2), 1
    murs   = pos_voisins(enter, lignes, colonnes)
    visite = [[False] * colonnes for i in range(lignes)]
    visite[enter[0]][enter[1]] = True
    res[enter[0]][enter[1]]    = 1

    while murs:
        pos_mur = murs.pop(random.randint(0, len(murs)-1))
        x, y = pos_mur
        if combien_visite(visite, pos_mur) == 1:
            res[x][y] = 1
            voisin = voisin_vide(res, pos_mur)
            diff = diff_vecteur(voisin, pos_mur)
            nouveau_libre_dir = neg_vecteur(diff)
            nouveau_libre = somme_vecteur(nouveau_libre_dir, pos_mur)
            for voisin in pos_voisins(nouveau_libre, lignes, colonnes):
                ajouter_murs_unique(murs, voisin, res)
            nlx, nly = nouveau_libre
            visite[nlx][nly] = True
            res[nlx][nly]    = 1
        visite[x][y] = True

    res[enter[0]][enter[1]] = 2

    res[ligne_sortie(res)][colonnes - 2] = 3

    return res


@click.command("generation")
@click.argument("lignes", type=int)
@click.argument("colonnes", type=int)
def main(lignes, colonnes):
    lab = generation(colonnes, lignes)
    printLab(lab)

if __name__ == "__main__":
    main()