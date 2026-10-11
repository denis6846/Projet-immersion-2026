#include "jeu/capture.h"
#include <stddef.h>
#include <stdio.h>
/* ============================================================
 * CONSTANTES
 * ============================================================ */

#define CASE_VIDE 0
#define CASE_CONQUISE_BLEU 1
#define CASE_CONQUISE_ROUGE -1
#define CASE_BARRICADE 4

/* Directions : haut, bas, gauche, droite */
static const int directions[4][2] = {
    {-1, 0},
    { 1, 0},
    { 0,-1},
    { 0, 1}
};

/* ============================================================
 * OUTILS
 * ============================================================ */

static int dans_plateau(int ligne, int colonne)
{
    return ligne >= 0 && ligne < ROWS &&
           colonne >= 0 && colonne < COLS;
}

int capture_est_piece(int valeur)
{
    return valeur == 2 ||
           valeur == 3 ||
           valeur == -2 ||
           valeur == -3;
}

int capture_est_piece_joueur(int valeur, int joueur)
{
    if (joueur == BLEU)
        return valeur == 2 || valeur == 3;

    if (joueur == ROUGE)
        return valeur == -2 || valeur == -3;

    return 0;
}

int capture_est_piece_adverse(int valeur, int joueur)
{
    if (joueur == BLEU)
        return valeur == -2 || valeur == -3;

    if (joueur == ROUGE)
        return valeur == 2 || valeur == 3;

    return 0;
}

/* ============================================================
 * CAPTURE LINCA INTERNE
 *
 * exclusion_direction :
 *   -1 = aucune exclusion
 *    0 = haut
 *    1 = bas
 *    2 = gauche
 *    3 = droite
 *
 * Lors d'un déplacement, la direction du déplacement
 * est réservée au SEULTŌU.
 * ============================================================ */

static int capture_linca_interne(
    int ligne,
    int colonne,
    int joueur,
    int exclusion_direction)
{
    int captures = 0;
    int d;

    if (!dans_plateau(ligne, colonne))
        return 0;

    /*
     * La case doit contenir une pièce du joueur.
     */
    if (!capture_est_piece_joueur(
            jeu_valeur_case(ligne, colonne),
            joueur))
        return 0;

    for (d = 0; d < 4; d++)
    {
        int dl;
        int dc;

        int ligne_ennemi;
        int colonne_ennemi;

        int ligne_soutien;
        int colonne_soutien;

        int valeur_ennemi;
        int valeur_soutien;

        /*
         * Cette direction est éventuellement réservée
         * au SEULTŌU.
         */
        if (d == exclusion_direction)
            continue;

        dl = directions[d][0];
        dc = directions[d][1];

        ligne_ennemi = ligne + dl;
        colonne_ennemi = colonne + dc;

        ligne_soutien = ligne + 2 * dl;
        colonne_soutien = colonne + 2 * dc;

        /*
         * Les deux cases doivent être dans le plateau.
         */
        if (!dans_plateau(
                ligne_ennemi,
                colonne_ennemi))
            continue;

        if (!dans_plateau(
                ligne_soutien,
                colonne_soutien))
            continue;

        valeur_ennemi =
            jeu_valeur_case(
                ligne_ennemi,
                colonne_ennemi);

        valeur_soutien =
            jeu_valeur_case(
                ligne_soutien,
                colonne_soutien);

        /*
         * La case intermédiaire doit contenir
         * une pièce adverse.
         */
        if (!capture_est_piece_adverse(
                valeur_ennemi,
                joueur))
            continue;

        /*
         * La case derrière doit contenir
         * une pièce du joueur.
         */
        if (!capture_est_piece_joueur(
                valeur_soutien,
                joueur))
            continue;

        /*
         * Capture de la pièce adverse.
         */
        printf(
            "LINCA : joueur=%d capture=(%d,%d) soutien=(%d,%d)\n",
            joueur,
            ligne_ennemi,
            colonne_ennemi,
            ligne_soutien,
            colonne_soutien
        );

        if (jeu_capturer_piece(
                ligne_ennemi,
                colonne_ennemi,
                joueur))
        {
            captures++;
        }
    }

    return captures;
}

/* ============================================================
 * LINCA
 *
 * JOUEUR | ADVERSAIRE | JOUEUR
 *
 * Les 4 directions sont testées lorsqu'on appelle
 * directement cette fonction.
 * ============================================================ */

int capture_linca(
    int ligne,
    int colonne,
    int joueur)
{
    return capture_linca_interne(
        ligne,
        colonne,
        joueur,
        -1
    );
}

/* ============================================================
 * SEULTŌU
 *
 * On regarde uniquement dans la direction
 * du déplacement qui vient d'être effectué.
 *
 * Exemple :
 *
 *       BLEU -> ROUGE -> ?
 *
 * Derrière ROUGE :
 *
 * - vide       -> capture
 * - ROUGE      -> capture
 * - BLEU       -> pas de capture
 * - ROI BLEU   -> pas de capture
 * - barricade  -> pas de capture
 *
 * Si la pièce adverse est au bord du plateau,
 * la capture est autorisée.
 * ============================================================ */

int capture_seultou(
    int ligne_depart,
    int colonne_depart,
    int ligne_arrivee,
    int colonne_arrivee,
    int joueur)
{
    int dl;
    int dc;

    int ligne_cible;
    int colonne_cible;

    int ligne_derriere;
    int colonne_derriere;

    int valeur_cible;
    int valeur_derriere;

    if (joueur != BLEU && joueur != ROUGE)
        return 0;

    /*
     * Calcul de la direction du déplacement.
     */
    dl = ligne_arrivee - ligne_depart;
    dc = colonne_arrivee - colonne_depart;

    /*
     * Le déplacement doit être horizontal ou vertical.
     */
    if (dl != 0 && dc != 0)
        return 0;

    if (dl == 0 && dc == 0)
        return 0;

    /*
     * On transforme le déplacement en direction.
     */
    if (dl > 0)
        dl = 1;
    else if (dl < 0)
        dl = -1;

    if (dc > 0)
        dc = 1;
    else if (dc < 0)
        dc = -1;

    /*
     * La cible SEULTŌU est juste derrière
     * la case d'arrivée.
     */
    ligne_cible = ligne_arrivee + dl;
    colonne_cible = colonne_arrivee + dc;

    /*
     * La cible doit être dans le plateau.
     */
    if (!dans_plateau(ligne_cible, colonne_cible))
        return 0;

    /*
     * La cible doit être une pièce adverse.
     */
    valeur_cible = jeu_valeur_case(
        ligne_cible,
        colonne_cible
    );

    if (!capture_est_piece_adverse(
            valeur_cible,
            joueur))
    {
        return 0;
    }

    /*
     * Case située derrière la cible.
     */
    ligne_derriere = ligne_cible + dl;
    colonne_derriere = colonne_cible + dc;

    /*
     * Si la cible est au bord du plateau,
     * la capture est autorisée.
     */
    if (!dans_plateau(
            ligne_derriere,
            colonne_derriere))
    {
        return jeu_capturer_piece(
            ligne_cible,
            colonne_cible,
            joueur
        );
    }

    /*
     * Valeur de la case derrière.
     */
    valeur_derriere = jeu_valeur_case(
        ligne_derriere,
        colonne_derriere
    );

    printf(
        "SEULTOU : joueur=%d cible=(%d,%d) valeur_cible=%d "
        "derriere=(%d,%d) valeur_derriere=%d\n",
        joueur,
        ligne_cible,
        colonne_cible,
        valeur_cible,
        ligne_derriere,
        colonne_derriere,
        valeur_derriere
    );

    /*
     * Toute pièce derrière la cible bloque SEULTŌU.
     *
     * BLEU :
     *   2 = pion
     *   3 = roi
     *
     * ROUGE :
     *   -2 = pion
     *   -3 = roi
     */
    if (capture_est_piece(valeur_derriere))
    {
        printf("SEULTOU BLOQUE : piece derriere\n");
        return 0;
    }

    /*
     * Une barricade bloque également.
     */
    if (valeur_derriere == CASE_BARRICADE)
    {
        printf("SEULTOU BLOQUE : barricade derriere\n");
        return 0;
    }

    /*
     * Case vide ou case conquise :
     * capture autorisée.
     */
    return jeu_capturer_piece(
        ligne_cible,
        colonne_cible,
        joueur
    );
}

/* ============================================================
 * CAPTURE GENERALE
 * ============================================================ */

int effectuer_captures(
    int ligne_depart,
    int colonne_depart,
    int ligne_arrivee,
    int colonne_arrivee,
    int joueur)
{
    int total = 0;

    int dl;
    int dc;

    int direction_seultou = -1;

    /*
     * Vérification du joueur.
     */
    if (joueur != BLEU && joueur != ROUGE)
        return 0;

    /*
     * Vérification des positions.
     */
    if (!dans_plateau(ligne_depart, colonne_depart) ||
        !dans_plateau(ligne_arrivee, colonne_arrivee))
        return 0;

    /*
     * La case d'arrivée doit contenir
     * une pièce du joueur.
     */
    if (!capture_est_piece_joueur(
            jeu_valeur_case(ligne_arrivee, colonne_arrivee),
            joueur))
        return 0;

    /*
     * Calcul du déplacement.
     */
    dl = ligne_arrivee - ligne_depart;
    dc = colonne_arrivee - colonne_depart;

    /*
     * Le déplacement doit être horizontal
     * ou vertical.
     */
    if (dl != 0 && dc != 0)
        return 0;

    /*
     * Détermination de la direction du SEULTŌU.
     *
     * 0 = haut
     * 1 = bas
     * 2 = gauche
     * 3 = droite
     */
    if (dl < 0)
        direction_seultou = 0;
    else if (dl > 0)
        direction_seultou = 1;
    else if (dc < 0)
        direction_seultou = 2;
    else if (dc > 0)
        direction_seultou = 3;

    /*
     * SEULTŌU
     *
     * Il est testé uniquement dans la direction
     * du déplacement.
     *
     * Exemple :
     *
     *       BLEU -> ROUGE -> BLEU
     *
     * Le SEULTŌU regarde le BLEU derrière ROUGE
     * et refuse la capture.
     */
    if (direction_seultou != -1)
    {
        total += capture_seultou(
            ligne_depart,
            colonne_depart,
            ligne_arrivee,
            colonne_arrivee,
            joueur
        );
    }

    /*
     * LINCA
     *
     * On teste les 4 directions, SAUF la direction
     * du déplacement.
     *
     * La direction du déplacement est réservée
     * exclusivement au SEULTŌU.
     */
    total += capture_linca_interne(
        ligne_arrivee,
        colonne_arrivee,
        joueur,
        direction_seultou
    );

    return total;
}