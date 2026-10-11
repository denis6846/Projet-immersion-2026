#include "jeu/jeu.h"
#include <stddef.h>

/* Toutes les pièces du jeu */
static Piece pieces[] = {

    /* BLEU */
    {0, 2, "♜", BLEU, TOUR},
    {0, 3, "♜", BLEU, TOUR},
    {1, 1, "♛", BLEU, ROI},
    {1, 2, "♜", BLEU, TOUR},
    {1, 3, "♜", BLEU, TOUR},
    {2, 0, "♜", BLEU, TOUR},
    {2, 1, "♜", BLEU, TOUR},
    {2, 2, "♜", BLEU, TOUR},
    {3, 0, "♜", BLEU, TOUR},
    {3, 1, "♜", BLEU, TOUR},

    /* ROUGE */
    {6, 8, "♜", ROUGE, TOUR},
    {6, 7, "♜", ROUGE, TOUR},
    {5, 9, "♛", ROUGE, ROI},
    {5, 8, "♜", ROUGE, TOUR},
    {5, 7, "♜", ROUGE, TOUR},
    {4, 10, "♜", ROUGE, TOUR},
    {4, 9, "♜", ROUGE, TOUR},
    {4, 8, "♜", ROUGE, TOUR},
    {3, 10, "♜", ROUGE, TOUR},
    {3, 9, "♜", ROUGE, TOUR}
};

static int nb_pieces =
    sizeof(pieces) / sizeof(pieces[0]);

static int current_player = BLEU;

/*
 * Garde en mémoire la couleur du dernier pion
 * ou roi qui est passé sur chaque case.
 *
 * -1 = case neutre
 * BLEU = case conquise par BLEU
 * ROUGE = case conquise par ROUGE
 */
static int conquises[ROWS][COLS];

/*
 * Garde en mémoire les cases bloquées par une barricade.
 *
 * 1 = barricade présente
 * 0 = pas de barricade
 */
static int barricades[ROWS][COLS];


/* ============================================================
 * INITIALISATION
 * ============================================================ */

void jeu_initialiser(void)
{
    int r;
    int c;

    current_player = BLEU;

    for (r = 0; r < ROWS; r = r + 1)
    {
        for (c = 0; c < COLS; c = c + 1)
        {
            conquises[r][c] = -1;
            barricades[r][c] = 0;
        }
    }
}


/* ============================================================
 * PIECES
 * ============================================================ */

int jeu_nombre_pieces(void)
{
    return nb_pieces;
}


Piece *jeu_piece(int index)
{
    if (index < 0 || index >= nb_pieces)
    {
        return NULL;
    }

    return &pieces[index];
}


int jeu_piece_at(int row, int col)
{
    int i;

    for (i = 0; i < nb_pieces; i = i + 1)
    {
        /*
         * Une pièce avec row < 0 est une pièce capturée.
         */
        if (pieces[i].row < 0)
        {
            continue;
        }

        if (pieces[i].row == row &&
            pieces[i].col == col)
        {
            return i;
        }
    }

    return -1;
}


/* ============================================================
 * JOUEUR
 * ============================================================ */

int jeu_joueur_actuel(void)
{
    return current_player;
}


void jeu_changer_joueur(void)
{
    current_player = 1 - current_player;
}


/* ============================================================
 * VERIFICATION DU CHEMIN
 * ============================================================ */

/*
 * Vérifie qu'il n'y a pas de pièce ni de barricade
 * entre le départ et l'arrivée.
 */
static int chemin_libre(
    Piece *piece,
    int new_row,
    int new_col)
{
    int dr = new_row - piece->row;
    int dc = new_col - piece->col;

    int step_row = 0;
    int step_col = 0;

    int row;
    int col;

    if (dr > 0)
    {
        step_row = 1;
    }
    else if (dr < 0)
    {
        step_row = -1;
    }

    if (dc > 0)
    {
        step_col = 1;
    }
    else if (dc < 0)
    {
        step_col = -1;
    }

    row = piece->row + step_row;
    col = piece->col + step_col;

    while (row != new_row ||
           col != new_col)
    {
        if (jeu_piece_at(row, col) != -1)
        {
            return 0;
        }

        if (barricades[row][col] == 1)
        {
            return 0;
        }

        row = row + step_row;
        col = col + step_col;
    }

    return 1;
}


/* ============================================================
 * VERIFICATION D'UN DEPLACEMENT
 * ============================================================ */

int jeu_peut_deplacer(
    int piece_index,
    int new_row,
    int new_col)
{
    Piece *piece;
    int dr;
    int dc;
    int destination;

    if (piece_index < 0 ||
        piece_index >= nb_pieces)
    {
        return 0;
    }

    if (new_row < 0 ||
        new_row >= ROWS ||
        new_col < 0 ||
        new_col >= COLS)
    {
        return 0;
    }

    /*
     * Impossible d'arriver sur une barricade.
     */
    if (barricades[new_row][new_col] == 1)
    {
        return 0;
    }

    piece = &pieces[piece_index];

    /*
     * Une pièce capturée ne peut plus être déplacée.
     */
    if (piece->row < 0)
    {
        return 0;
    }

    /*
     * Une pièce ne peut être déplacée
     * que pendant le tour de son joueur.
     */
    if (piece->color != current_player)
    {
        return 0;
    }

    dr = new_row - piece->row;
    dc = new_col - piece->col;

    /*
     * Impossible de rester sur place.
     */
    if (dr == 0 && dc == 0)
    {
        return 0;
    }

    /*
     * TOUR et ROI se déplacent uniquement
     * en ligne ou en colonne.
     */
    if (piece->type == TOUR ||
        piece->type == ROI)
    {
        if (dr != 0 && dc != 0)
        {
            return 0;
        }
    }

    /*
     * Vérifie que le chemin est libre.
     */
    if (chemin_libre(
            piece,
            new_row,
            new_col) == 0)
    {
        return 0;
    }

    /*
     * Vérifie la case d'arrivée.
     */
    destination =
        jeu_piece_at(
            new_row,
            new_col);

    /*
     * Une case occupée par une pièce,
     * alliée ou adverse, interdit le déplacement.
     *
     * Les captures sont gérées uniquement
     * par les règles de capture :
     * LINCA et SEULTŌU.
     */
    if (destination != -1)
    {
        return 0;
    }

    /*
     * La case est libre.
     */
    return 1;
}


/* ============================================================
 * CAPTURE D'UNE PIECE
 * ============================================================ */

int jeu_capturer_piece(
    int ligne,
    int colonne,
    int joueur)
{
    int index;

    if (ligne < 0 ||
        ligne >= ROWS ||
        colonne < 0 ||
        colonne >= COLS)
    {
        return 0;
    }

    if (joueur != BLEU &&
        joueur != ROUGE)
    {
        return 0;
    }

    /*
     * Une barricade ne peut jamais être capturée.
     */
    if (barricades[ligne][colonne] == 1)
    {
        return 0;
    }

    index =
        jeu_piece_at(
            ligne,
            colonne);

    /*
     * Pas de pièce à cette position.
     */
    if (index == -1)
    {
        return 0;
    }

    /*
     * Une pièce alliée ne peut pas être capturée.
     */
    if (pieces[index].color == joueur)
    {
        return 0;
    }

    /*
     * La pièce est retirée du plateau.
     */
    pieces[index].row = -1;
    pieces[index].col = -1;

    /*
     * La case de la pièce capturée
     * redevient neutre.
     */
    conquises[ligne][colonne] = -1;

    return 1;
}


/* ============================================================
 * DEPLACEMENT EFFECTIF
 * ============================================================ */

int jeu_deplacer(
    int piece_index,
    int new_row,
    int new_col)
{
    Piece *piece;

    /*
     * Vérifie toutes les règles du déplacement.
     */
    if (jeu_peut_deplacer(
            piece_index,
            new_row,
            new_col) == 0)
    {
        return 0;
    }

    piece = &pieces[piece_index];

    /*
     * La destination a déjà été vérifiée
     * comme étant libre par jeu_peut_deplacer().
     *
     * Aucune capture directe n'est donc effectuée ici.
     */

    /*
     * Déplacement de la pièce.
     */
    piece->row = new_row;
    piece->col = new_col;

    /*
     * La case d'arrivée devient conquise
     * par la pièce qui vient d'y arriver.
     */
    conquises[new_row][new_col] =
        piece->color;

    /*
     * Changement de joueur.
     */
    current_player =
        1 - current_player;

    return 1;
}


/* ============================================================
 * VALEUR D'UNE CASE
 * ============================================================ */

int jeu_valeur_case(
    int row,
    int col)
{
    int index;
    Piece *p;
    int val;

    index =
        jeu_piece_at(
            row,
            col);

    /*
     * Une barricade vaut 4.
     */
    if (barricades[row][col] == 1)
    {
        return 4;
    }

    /*
     * Une pièce :
     *
     * TOUR = 2 / -2
     * ROI  = 3 / -3
     */
    if (index != -1)
    {
        p = &pieces[index];

        if (p->type == ROI)
        {
            val = 3;
        }
        else
        {
            val = 2;
        }

        if (p->color == BLEU)
        {
            return val;
        }
        else
        {
            return -val;
        }
    }

    /*
     * Case conquise par BLEU.
     */
    if (conquises[row][col] == BLEU)
    {
        return 1;
    }

    /*
     * Case conquise par ROUGE.
     */
    if (conquises[row][col] == ROUGE)
    {
        return -1;
    }

    /*
     * Case neutre.
     */
    return 0;
}


/* ============================================================
 * CONQUETE
 * ============================================================ */

int jeu_couleur_conquete(
    int row,
    int col)
{
    if (conquises[row][col] == BLEU)
    {
        return 1;
    }

    if (conquises[row][col] == ROUGE)
    {
        return -1;
    }

    return 0;
}


/* ============================================================
 * BARRICADES
 * ============================================================ */

void jeu_poser_barricade(
    int row,
    int col)
{
    if (row < 0 ||
        row >= ROWS ||
        col < 0 ||
        col >= COLS)
    {
        return;
    }

    barricades[row][col] = 1;
}


int jeu_est_barricade(
    int row,
    int col)
{
    return barricades[row][col];
}


int jeu_roi_dans_camp_adverse(int joueur)
{
    for (int i = 0; i < ROWS; i++)
    {
        for (int j = 0; j < COLS; j++)
        {
            int valeur = jeu_valeur_case(i, j);

            if (joueur == BLEU && valeur == 3)
            {
                /*
                 * Camp ROUGE
                 */
                if ((i == 3 && j >= 8) ||
                    (i == 4 && j >= 7) ||
                    (i == 5 && j >= 7) ||
                    (i == 6 && j >= 6))
                {
                    return 1;
                }
            }

            if (joueur == ROUGE && valeur == -3)
            {
                /*
                 * Camp BLEU
                 */
                if ((i == 0 && j >= 2) ||
                    (i == 1 && j >= 1 && j <= 3) ||
                    (i == 2 && j >= 0 && j <= 2) ||
                    (i == 3 && j >= 0 && j <= 1))
                {
                    return 1;
                }
            }
        }
    }

    return 0;
}