#include "IA/STUPID_AI.h"

#include <stdlib.h>
#include <stdio.h>


/* ============================================================
 * RECHERCHE DES PIECES D'UN JOUEUR
 * ============================================================ */

pawnSet searchPawn(
    int player,
    int board[ROWS][COLS])
{
    pawnSet pos;

    pos.pawnCount = 0;


    for (int row = 0; row < ROWS; row++)
    {
        for (int col = 0; col < COLS; col++)
        {
            /*
             * BLEU :
             * tour = 2
             * roi  = 3
             */

            if (player == BLEU &&
                (board[row][col] == 2 ||
                 board[row][col] == 3))
            {
                if (pos.pawnCount < 10)
                {
                    pos.pawnNumber[pos.pawnCount].row = row;
                    pos.pawnNumber[pos.pawnCount].col = col;

                    pos.pawnCount++;
                }
            }


            /*
             * ROUGE :
             * tour = -2
             * roi  = -3
             */

            if (player == ROUGE &&
                (board[row][col] == -2 ||
                 board[row][col] == -3))
            {
                if (pos.pawnCount < 10)
                {
                    pos.pawnNumber[pos.pawnCount].row = row;
                    pos.pawnNumber[pos.pawnCount].col = col;

                    pos.pawnCount++;
                }
            }
        }
    }


    return pos;
}


/* ============================================================
 * CHOIX D'UN COUP ALÉATOIRE
 * ============================================================ */

PawnMoveProposition randomMove(
    int board[ROWS][COLS],
    int player)
{
    pawnSet availablePawns =
        searchPawn(player, board);


    /*
     * Aucun pion disponible.
     */

    if (availablePawns.pawnCount == 0)
    {
        return (PawnMoveProposition){
            -1,
            -1,
            -1,
            -1
        };
    }


    /*
     * Construire la liste des pièces
     * qui possèdent au moins un mouvement.
     */

    pawnSet movablePawns;

    movablePawns.pawnCount = 0;


    for (int i = 0;
         i < availablePawns.pawnCount;
         i++)
    {
        MoveSet moves =
            generatePawnMoves(
                board,
                availablePawns.pawnNumber[i]
            );


        if (moves.count > 0)
        {
            movablePawns.pawnNumber[
                movablePawns.pawnCount
            ] =
                availablePawns.pawnNumber[i];

            movablePawns.pawnCount++;
        }
    }


    /*
     * Toutes les pièces sont bloquées.
     */

    if (movablePawns.pawnCount == 0)
    {
        return (PawnMoveProposition){
            -1,
            -1,
            -1,
            -1
        };
    }


    /*
     * Choisir une pièce aléatoire
     * parmi les pièces pouvant jouer.
     */

    int piece_index =
        rand() % movablePawns.pawnCount;


    Positions piece =
        movablePawns.pawnNumber[piece_index];


    /*
     * Générer ses mouvements.
     */

    MoveSet availableMoves =
        generatePawnMoves(
            board,
            piece
        );


    /*
     * Choisir une destination aléatoire.
     */

    int move_index =
        rand() % availableMoves.count;


    Positions destination =
        availableMoves.moves[move_index];


    return (PawnMoveProposition){
        piece.row,
        piece.col,
        destination.row,
        destination.col
    };
}