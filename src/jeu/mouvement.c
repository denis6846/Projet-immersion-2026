#include "jeu/mouvement.h"

/**
 * @brief Génère les mouvements possibles d'une pièce.
 *
 * La pièce peut se déplacer horizontalement ou verticalement.
 * Le déplacement s'arrête dès qu'une pièce ou une barricade est rencontrée.
 *
 * @param board Plateau de jeu.
 * @param pawn Position de la pièce.
 * @return MoveSet Ensemble des cases accessibles.
 */
MoveSet generatePawnMoves(int board[7][11], Positions pawn)
{
    MoveSet setOfMoves;
    setOfMoves.count = 0;

    /*
     * BAS
     */
    for (int i = 1; pawn.row + i < 7; i++)
    {
        int row = pawn.row + i;
        int col = pawn.col;

        /*
         * Une pièce ou une barricade bloque le déplacement.
         */
        if (board[row][col] < -1 || board[row][col] > 1)
            break;

        setOfMoves.moves[setOfMoves.count].row = row;
        setOfMoves.moves[setOfMoves.count].col = col;
        setOfMoves.count++;
    }

    /*
     * DROITE
     */
    for (int i = 1; pawn.col + i < 11; i++)
    {
        int row = pawn.row;
        int col = pawn.col + i;

        if (board[row][col] < -1 || board[row][col] > 1)
            break;

        setOfMoves.moves[setOfMoves.count].row = row;
        setOfMoves.moves[setOfMoves.count].col = col;
        setOfMoves.count++;
    }

    /*
     * HAUT
     */
    for (int i = 1; pawn.row - i >= 0; i++)
    {
        int row = pawn.row - i;
        int col = pawn.col;

        if (board[row][col] < -1 || board[row][col] > 1)
            break;

        setOfMoves.moves[setOfMoves.count].row = row;
        setOfMoves.moves[setOfMoves.count].col = col;
        setOfMoves.count++;
    }

    /*
     * GAUCHE
     */
    for (int i = 1; pawn.col - i >= 0; i++)
    {
        int row = pawn.row;
        int col = pawn.col - i;

        if (board[row][col] < -1 || board[row][col] > 1)
            break;

        setOfMoves.moves[setOfMoves.count].row = row;
        setOfMoves.moves[setOfMoves.count].col = col;
        setOfMoves.count++;
    }

    return setOfMoves;
}

/**
 * @brief Vérifie si une position appartient à un ensemble de mouvements.
 *
 * @param pawnPosition Position à vérifier.
 * @param setOfMoves Ensemble des mouvements possibles.
 * @return 1 si la position est légale, 0 sinon.
 */
int isLegalMove(Positions pawnPosition, MoveSet setOfMoves)
{
    for (int i = 0; i < setOfMoves.count; i++)
    {
        if (pawnPosition.row == setOfMoves.moves[i].row &&
            pawnPosition.col == setOfMoves.moves[i].col)
        {
            return 1;
        }
    }

    return 0;
}

/**
 * @brief Fonction conservée pour compatibilité.
 *
 * Cette fonction n'est pas utilisée dans le projet.
 *
 * @param board Plateau de jeu.
 * @param PawnProp Proposition de déplacement.
 */
void movePawn(int board[7][11], PawnMoveProposition PawnProp)
{
    (void)board;
    (void)PawnProp;
}