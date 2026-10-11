#include "jeu/checkScore.h"
#include "jeu/jeu.h"

#include <stdio.h>

static int roi_dans_camp_adverse(int joueur);


/* =========================
 * CALCUL DU SCORE
 * ========================= */

result giveScore(void)
{
    int scoreCountBlue = 0;
    int scoreCountRed = 0;

    /*
     * Parcours du véritable plateau du jeu.
     */
    for (int i = 0; i < ROWS; i++)
    {
        for (int j = 0; j < COLS; j++)
        {
            int valeur = jeu_valeur_case(i, j);

            /*
             * BLEU
             *
             * 1  = case conquise par BLEU -> 1 point
             * 3  = roi BLEU              -> 1 point
             * 2  = pion BLEU             -> 2 points
             */
            if (valeur == 1 || valeur == 3)
            {
                scoreCountBlue++;
            }
            else if (valeur == 2)
            {
                scoreCountBlue += 2;
            }

            /*
             * ROUGE
             *
             * -1 = case conquise par ROUGE -> 1 point
             * -3 = roi ROUGE              -> 1 point
             * -2 = pion ROUGE             -> 2 points
             */
            if (valeur == -1 || valeur == -3)
            {
                scoreCountRed++;
            }
            else if (valeur == -2)
            {
                scoreCountRed += 2;
            }
        }
    }

    return (result){
        scoreCountBlue,
        scoreCountRed
    };
}


/* =========================
 * VERIFICATION DE VICTOIRE
 * ========================= */

int checkWin(int round)
{
    /*
     * Victoire immédiate :
     * le roi atteint le camp adverse.
     */
    if (roi_dans_camp_adverse(BLEU))
    {
        return VICTOIRE_BLEU;
    }

    if (roi_dans_camp_adverse(ROUGE))
    {
        return VICTOIRE_ROUGE;
    }


    /*
     * Vérification de la présence des rois.
     */
    int isKingAliveRed = 0;
    int isKingAliveBlue = 0;

    for (int i = 0; i < ROWS; i++)
    {
        for (int j = 0; j < COLS; j++)
        {
            int valeur = jeu_valeur_case(i, j);

            if (valeur == 3)
            {
                isKingAliveBlue = 1;
            }
            else if (valeur == -3)
            {
                isKingAliveRed = 1;
            }
        }
    }

    /*
     * Le roi BLEU a été capturé.
     */
    if (!isKingAliveBlue)
    {
        return VICTOIRE_ROUGE;
    }

    /*
     * Le roi ROUGE a été capturé.
     */
    if (!isKingAliveRed)
    {
        return VICTOIRE_BLEU;
    }


    /*
     * Fin de partie après 64 tours.
     */
    if (round >= 64)
    {
        result score = giveScore();

        if (score.blueScore > score.redScore)
        {
            return VICTOIRE_BLEU;
        }

        if (score.redScore > score.blueScore)
        {
            return VICTOIRE_ROUGE;
        }

        return EGALITE;
    }


    /*
     * La partie continue.
     */
    return PARTIE_EN_COURS;
}


/* =========================
 * ROI DANS LE CAMP ADVERSE
 * ========================= */

static int roi_dans_camp_adverse(int joueur)
{
    for (int i = 0; i < ROWS; i++)
    {
        for (int j = 0; j < COLS; j++)
        {
            int valeur = jeu_valeur_case(i, j);

            /*
             * Roi BLEU dans le camp ROUGE.
             * Le camp ROUGE est uniquement la case
             * en bas à droite : (6,10).
             */
            if (joueur == BLEU &&
                valeur == 3 &&
                i == 6 &&
                j == 10)
            {
                return 1;
            }

            /*
             * Roi ROUGE dans le camp BLEU.
             * Le camp BLEU est uniquement la case
             * en haut à gauche : (0,0).
             */
            if (joueur == ROUGE &&
                valeur == -3 &&
                i == 0 &&
                j == 0)
            {
                return 1;
            }
        }
    }

    return 0;
}