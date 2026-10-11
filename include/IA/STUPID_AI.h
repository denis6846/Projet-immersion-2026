#ifndef STUPID_AI_H
#define STUPID_AI_H

#include "../jeu/mouvement.h"
#include "../jeu/jeu.h"

typedef struct
{
    Positions pawnNumber[10];
    int pawnCount;
} pawnSet;

pawnSet searchPawn(int player, int board[ROWS][COLS]);

PawnMoveProposition randomMove(
    int board[ROWS][COLS],
    int player
);

#endif