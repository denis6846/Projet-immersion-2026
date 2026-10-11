#include <gtk/gtk.h>
#include <cairo.h>
#include <glib-unix.h>

#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

#include "jeu/jeu.h"
#include "jeu/capture.h"
#include "jeu/checkScore.h"
#include "jeu/mouvement.h"

#include "GUI/interface.h"

#include "network/joueur.h"
#include "network/server.h"
#include "network/protocole.h"

#include "IA/STUPID_AI.h"

/* ============================================================

* MODES
* ============================================================ */

typedef enum {
MODE_LOCAL,
MODE_CLIENT,
MODE_SERVEUR
} InterfaceMode;

/* ============================================================

* DONNÉES DE L'INTERFACE
* ============================================================ */

typedef struct {
InterfaceMode mode;


int joueur;

int socket;
int server_socket;

GtkWidget *drawing_area;

int round;
int game_result;

bool ia_active;


} InterfaceData;

/* ============================================================

* PROTOTYPES
* ============================================================ */

static gboolean network_callback(
gint fd,
GIOCondition condition,
gpointer user_data
);

static gboolean server_accept_callback(
gint fd,
GIOCondition condition,
gpointer user_data
);

static gboolean server_network_callback(
gint fd,
GIOCondition condition,
gpointer user_data
);

static void activate(
GtkApplication *app,
gpointer user_data
);

static void verifier_fin_partie(
InterfaceData *data
);

static void ia_poser_barricade_serveur(
InterfaceData *data
);

static void ia_jouer_coup_serveur(
InterfaceData *data
);

static void ia_poser_barricade_client(
InterfaceData *data
);

static void ia_jouer_coup_client(
InterfaceData *data
);

/* ============================================================

* CROIX
* ============================================================ */

static int x1_col = -1;
static int x1_row = -1;

static int x2_col = -1;
static int x2_row = -1;

static bool croix_posee_j1 = false;
static bool croix_posee_j2 = false;

/* ============================================================

* PIÈCE SÉLECTIONNÉE
* ============================================================ */

static int selected_piece = -1;

/* ============================================================

* COORDONNÉES
* ============================================================ */

typedef struct {
int row;
int col;
} Coord;

/* ============================================================

* CASES AUTORISÉES BLEU
* ============================================================ */

static const Coord CASES_AUTORISEES_BLEU[] = {
{0, 4}, {0, 5}, {0, 6}, {0, 7},


{1, 4}, {1, 5}, {1, 6},

{2, 3}, {2, 4}, {2, 5},

{3, 2}, {3, 3}, {3, 4},

{4, 0}, {4, 1}, {4, 2}, {4, 3},

{5, 0}, {5, 1}, {5, 2}


};

static const int NB_CASES_BLEU =
sizeof(CASES_AUTORISEES_BLEU) /
sizeof(CASES_AUTORISEES_BLEU[0]);

/* ============================================================

* CASES AUTORISÉES ROUGE
* ============================================================ */

static const Coord CASES_AUTORISEES_ROUGE[] = {
{1, 10}, {1, 9}, {1, 8},


{2, 10}, {2, 9}, {2, 8}, {2, 7},

{3, 8}, {3, 7}, {3, 6},

{4, 7}, {4, 6}, {4, 5},

{5, 4}, {5, 5}, {5, 6},

{6, 6}, {6, 5}, {6, 4}, {6, 3}


};

static const int NB_CASES_ROUGE =
sizeof(CASES_AUTORISEES_ROUGE) /
sizeof(CASES_AUTORISEES_ROUGE[0]);

/* ============================================================

* OUTILS
* ============================================================ */

static bool est_dans_tableau(
int row,
int col,
const Coord *tableau,
int taille
)
{
for (int i = 0; i < taille; i++) {


    if (tableau[i].row == row &&
        tableau[i].col == col) {

        return true;
    }
}

return false;


}

/* ============================================================

* FIN DE PARTIE
* ============================================================ */

static void verifier_fin_partie(
InterfaceData *data
)
{
if (data == NULL)
return;


if (data->game_result != PARTIE_EN_COURS)
    return;

/*
 * Les 64 tours correspondent aux 64 déplacements
 * effectués par les deux joueurs.
 */

if (data->round >= 64) {

    result score =
        giveScore();

    printf(
        "64 tours atteints.\n"
    );

    printf(
        "Score BLEU  : %d\n",
        score.blueScore
    );

    printf(
        "Score ROUGE : %d\n",
        score.redScore
    );

    if (score.blueScore > score.redScore) {

        data->game_result =
            VICTOIRE_BLEU;

    }
    else if (score.redScore > score.blueScore) {

        data->game_result =
            VICTOIRE_ROUGE;

    }
    else {

        data->game_result =
            EGALITE;
    }
}
else {

    data->game_result =
        checkWin(data->round);
}

if (data->game_result != PARTIE_EN_COURS) {

    printf(
        "Partie terminée : résultat = %d\n",
        data->game_result
    );

    if (data->drawing_area != NULL) {

        gtk_widget_queue_draw(
            data->drawing_area
        );
    }
}


}

/* ============================================================

* DESSIN D'UNE PIÈCE
* ============================================================ */

static void draw_piece(
cairo_t *cr,
const char *symbol,
int color,
double x,
double y,
double size
)
{
if (color == BLEU) {


    cairo_set_source_rgb(
        cr,
        0.0,
        0.0,
        1.0
    );

} else {

    cairo_set_source_rgb(
        cr,
        1.0,
        0.0,
        0.0
    );
}

cairo_select_font_face(
    cr,
    "DejaVu Sans",
    CAIRO_FONT_SLANT_NORMAL,
    CAIRO_FONT_WEIGHT_NORMAL
);

cairo_set_font_size(
    cr,
    size * 0.72
);

cairo_text_extents_t extents;

cairo_text_extents(
    cr,
    symbol,
    &extents
);

double tx =
    x +
    (size - extents.width) / 2.0
    - extents.x_bearing;

double ty =
    y +
    (size - extents.height) / 2.0
    - extents.y_bearing;

cairo_move_to(
    cr,
    tx,
    ty
);

cairo_show_text(
    cr,
    symbol
);


}

/* ============================================================

* DESSIN D'UNE CROIX
* ============================================================ */

static void draw_croix(
cairo_t *cr,
int col,
int row,
double cell_w,
double cell_h
)
{
if (col == -1 || row == -1)
return;


cairo_set_source_rgb(
    cr,
    0.0,
    0.0,
    0.0
);

double font_size =
    MIN(cell_w, cell_h) * 0.7;

cairo_set_font_size(
    cr,
    font_size
);

cairo_text_extents_t extents;

cairo_text_extents(
    cr,
    "X",
    &extents
);

double center_x =
    col * cell_w +
    cell_w / 2.0;

double center_y =
    row * cell_h +
    cell_h / 2.0;

double tx =
    center_x -
    (extents.width / 2.0 +
     extents.x_bearing);

double ty =
    center_y -
    (extents.height / 2.0 +
     extents.y_bearing);

cairo_move_to(
    cr,
    tx,
    ty
);

cairo_show_text(
    cr,
    "X"
);


}

/* ============================================================

* DESSIN DU PLATEAU
* ============================================================ */

static void draw_board(
GtkDrawingArea *area,
cairo_t *cr,
int width,
int height,
gpointer user_data
)
{
(void)area;


InterfaceData *data =
    (InterfaceData *)user_data;

double cell_w =
    (double)width / COLS;

double cell_h =
    (double)height / ROWS;


/* ========================================================
 * FOND BLANC
 * ======================================================== */

cairo_set_source_rgb(
    cr,
    1.0,
    1.0,
    1.0
);

cairo_paint(cr);


/* ========================================================
 * CASES JAUNES
 * ======================================================== */

cairo_set_source_rgb(
    cr,
    1.0,
    0.82,
    0.28
);

cairo_rectangle(
    cr,
    8 * cell_w,
    0 * cell_h,
    cell_w,
    cell_h
);

cairo_fill(cr);

cairo_rectangle(
    cr,
    9 * cell_w,
    0 * cell_h,
    cell_w,
    cell_h
);

cairo_fill(cr);

cairo_rectangle(
    cr,
    10 * cell_w,
    0 * cell_h,
    cell_w,
    cell_h
);

cairo_fill(cr);

cairo_rectangle(
    cr,
    7 * cell_w,
    1 * cell_h,
    cell_w,
    cell_h
);

cairo_fill(cr);

cairo_rectangle(
    cr,
    6 * cell_w,
    2 * cell_h,
    cell_w,
    cell_h
);

cairo_fill(cr);

cairo_rectangle(
    cr,
    5 * cell_w,
    3 * cell_h,
    cell_w,
    cell_h
);

cairo_fill(cr);

cairo_rectangle(
    cr,
    4 * cell_w,
    4 * cell_h,
    cell_w,
    cell_h
);

cairo_fill(cr);

cairo_rectangle(
    cr,
    3 * cell_w,
    5 * cell_h,
    cell_w,
    cell_h
);

cairo_fill(cr);

cairo_rectangle(
    cr,
    2 * cell_w,
    6 * cell_h,
    cell_w,
    cell_h
);

cairo_fill(cr);

cairo_rectangle(
    cr,
    1 * cell_w,
    6 * cell_h,
    cell_w,
    cell_h
);

cairo_fill(cr);

cairo_rectangle(
    cr,
    0 * cell_w,
    6 * cell_h,
    cell_w,
    cell_h
);

cairo_fill(cr);


/* ========================================================
 * CASE BLEUE
 * ======================================================== */

cairo_set_source_rgb(
    cr,
    0.0,
    0.0,
    1.0
);

cairo_rectangle(
    cr,
    0 * cell_w,
    0 * cell_h,
    cell_w,
    cell_h
);

cairo_fill(cr);


/* Zone bleue claire */

for (int c = 2; c < 4; c++) {

    cairo_set_source_rgb(
        cr,
        0.75,
        0.84,
        0.90
    );

    cairo_rectangle(
        cr,
        c * cell_w,
        0 * cell_h,
        cell_w,
        cell_h
    );

    cairo_fill(cr);
}


for (int c = 1; c < 4; c++) {

    cairo_set_source_rgb(
        cr,
        0.75,
        0.84,
        0.90
    );

    cairo_rectangle(
        cr,
        c * cell_w,
        1 * cell_h,
        cell_w,
        cell_h
    );

    cairo_fill(cr);
}


for (int c = 0; c < 3; c++) {

    cairo_set_source_rgb(
        cr,
        0.75,
        0.84,
        0.90
    );

    cairo_rectangle(
        cr,
        c * cell_w,
        2 * cell_h,
        cell_w,
        cell_h
    );

    cairo_fill(cr);
}


for (int c = 0; c < 2; c++) {

    cairo_set_source_rgb(
        cr,
        0.75,
        0.84,
        0.90
    );

    cairo_rectangle(
        cr,
        c * cell_w,
        3 * cell_h,
        cell_w,
        cell_h
    );

    cairo_fill(cr);
}


/* ========================================================
 * CASE ROUGE
 * ======================================================== */

cairo_set_source_rgb(
    cr,
    1.0,
    0.0,
    0.0
);

cairo_rectangle(
    cr,
    10 * cell_w,
    6 * cell_h,
    cell_w,
    cell_h
);

cairo_fill(cr);


/* Zone rouge claire */

for (int c = 9; c < 11; c++) {

    cairo_set_source_rgb(
        cr,
        0.95,
        0.70,
        0.70
    );

    cairo_rectangle(
        cr,
        c * cell_w,
        3 * cell_h,
        cell_w,
        cell_h
    );

    cairo_fill(cr);
}


for (int c = 8; c < 11; c++) {

    cairo_set_source_rgb(
        cr,
        0.95,
        0.70,
        0.70
    );

    cairo_rectangle(
        cr,
        c * cell_w,
        4 * cell_h,
        cell_w,
        cell_h
    );

    cairo_fill(cr);
}


for (int c = 7; c < 10; c++) {

    cairo_set_source_rgb(
        cr,
        0.95,
        0.70,
        0.70
    );

    cairo_rectangle(
        cr,
        c * cell_w,
        5 * cell_h,
        cell_w,
        cell_h
    );

    cairo_fill(cr);
}


for (int c = 7; c < 9; c++) {

    cairo_set_source_rgb(
        cr,
        0.95,
        0.70,
        0.70
    );

    cairo_rectangle(
        cr,
        c * cell_w,
        6 * cell_h,
        cell_w,
        cell_h
    );

    cairo_fill(cr);
}


/* ========================================================
 * CASES CONQUISES
 * ======================================================== */

for (int row = 0;
     row < ROWS;
     row++) {

    for (int col = 0;
         col < COLS;
         col++) {

        int val =
            jeu_valeur_case(
                row,
                col
            );

        if (val == 1 ||
            val == 2 ||
            val == 3) {

            cairo_set_source_rgb(
                cr,
                0.75,
                0.84,
                0.90
            );

        }
        else if (val == -1 ||
                 val == -2 ||
                 val == -3) {

            cairo_set_source_rgb(
                cr,
                0.95,
                0.70,
                0.70
            );

        }
        else {

            continue;
        }

        cairo_rectangle(
            cr,
            col * cell_w,
            row * cell_h,
            cell_w,
            cell_h
        );

        cairo_fill(cr);
    }
}


/* ========================================================
 * CASE SÉLECTIONNÉE + COUPS POSSIBLES
 * ======================================================== */

if (selected_piece != -1) {

    Piece *p =
        jeu_piece(selected_piece);

    if (p != NULL &&
        p->row >= 0) {

        cairo_set_source_rgb(
            cr,
            0.6,
            1.0,
            0.6
        );

        cairo_rectangle(
            cr,
            p->col * cell_w,
            p->row * cell_h,
            cell_w,
            cell_h
        );

        cairo_fill(cr);


        /* HAUT */

        for (int row = p->row - 1;
             row >= 0;
             row--) {

            if (jeu_piece_at(
                    row,
                    p->col
                ) != -1 ||
                jeu_est_barricade(
                    row,
                    p->col
                ))
                break;

            cairo_set_source_rgb(
                cr,
                0.6,
                1.0,
                0.6
            );

            cairo_rectangle(
                cr,
                p->col * cell_w,
                row * cell_h,
                cell_w,
                cell_h
            );

            cairo_fill(cr);
        }


        /* BAS */

        for (int row = p->row + 1;
             row < ROWS;
             row++) {

            if (jeu_piece_at(
                    row,
                    p->col
                ) != -1 ||
                jeu_est_barricade(
                    row,
                    p->col
                ))
                break;

            cairo_set_source_rgb(
                cr,
                0.6,
                1.0,
                0.6
            );

            cairo_rectangle(
                cr,
                p->col * cell_w,
                row * cell_h,
                cell_w,
                cell_h
            );

            cairo_fill(cr);
        }


        /* GAUCHE */

        for (int col = p->col - 1;
             col >= 0;
             col--) {

            if (jeu_piece_at(
                    p->row,
                    col
                ) != -1 ||
                jeu_est_barricade(
                    p->row,
                    col
                ))
                break;

            cairo_set_source_rgb(
                cr,
                0.6,
                1.0,
                0.6
            );

            cairo_rectangle(
                cr,
                col * cell_w,
                p->row * cell_h,
                cell_w,
                cell_h
            );

            cairo_fill(cr);
        }


        /* DROITE */

        for (int col = p->col + 1;
             col < COLS;
             col++) {

            if (jeu_piece_at(
                    p->row,
                    col
                ) != -1 ||
                jeu_est_barricade(
                    p->row,
                    col
                ))
                break;

            cairo_set_source_rgb(
                cr,
                0.6,
                1.0,
                0.6
            );

            cairo_rectangle(
                cr,
                col * cell_w,
                p->row * cell_h,
                cell_w,
                cell_h
            );

            cairo_fill(cr);
        }
    }
}


/* ========================================================
 * GRILLE
 * ======================================================== */

cairo_set_source_rgb(
    cr,
    0.15,
    0.15,
    0.15
);

cairo_set_line_width(
    cr,
    1.0
);

for (int col = 0;
     col <= COLS;
     col++) {

    double x =
        col * cell_w;

    cairo_move_to(
        cr,
        x,
        0
    );

    cairo_line_to(
        cr,
        x,
        height
    );
}

for (int row = 0;
     row <= ROWS;
     row++) {

    double y =
        row * cell_h;

    cairo_move_to(
        cr,
        0,
        y
    );

    cairo_line_to(
        cr,
        width,
        y
    );
}

cairo_stroke(cr);


/* ========================================================
 * CROIX
 * ======================================================== */

draw_croix(
    cr,
    x1_col,
    x1_row,
    cell_w,
    cell_h
);

draw_croix(
    cr,
    x2_col,
    x2_row,
    cell_w,
    cell_h
);


/* ========================================================
 * PIÈCES
 * ======================================================== */

int nb_pieces =
    jeu_nombre_pieces();

for (int i = 0;
     i < nb_pieces;
     i++) {

    Piece *p =
        jeu_piece(i);

    if (p == NULL ||
        p->row < 0)
        continue;

    double x =
        p->col * cell_w;

    double y =
        p->row * cell_h;

    draw_piece(
        cr,
        p->symbol,
        p->color,
        x,
        y,
        MIN(cell_w, cell_h)
    );
}


/* ========================================================
 * CARACTÈRES CHINOIS
 * ======================================================== */

cairo_set_source_rgb(
    cr,
    1.0,
    1.0,
    1.0
);

cairo_select_font_face(
    cr,
    "Noto Sans CJK SC",
    CAIRO_FONT_SLANT_NORMAL,
    CAIRO_FONT_WEIGHT_BOLD
);

cairo_set_font_size(
    cr,
    60
);

cairo_move_to(
    cr,
    734,
    485
);

cairo_show_text(
    cr,
    "市"
);

cairo_move_to(
    cr,
    7,
    60
);

cairo_show_text(
    cr,
    "市"
);


/* ========================================================
 * ÉCRAN DE FIN
 * ======================================================== */

if (data != NULL &&
    data->game_result != PARTIE_EN_COURS) {

    cairo_set_source_rgba(
        cr,
        0.0,
        0.0,
        0.0,
        0.70
    );

    cairo_rectangle(
        cr,
        0,
        0,
        width,
        height
    );

    cairo_fill(cr);

    cairo_set_source_rgb(
        cr,
        1.0,
        1.0,
        1.0
    );

    cairo_select_font_face(
        cr,
        "DejaVu Sans",
        CAIRO_FONT_SLANT_NORMAL,
        CAIRO_FONT_WEIGHT_BOLD
    );

    cairo_set_font_size(
        cr,
        42
    );

    const char *texte =
        "PARTIE TERMINEE";

    if (data->game_result == EGALITE) {

        texte = "EGALITE";

    }
    else if (data->mode == MODE_LOCAL) {

        if (data->game_result == VICTOIRE_BLEU)
            texte = "VICTOIRE BLEU";

        else if (data->game_result == VICTOIRE_ROUGE)
            texte = "VICTOIRE ROUGE";
    }
    else {

        if (data->game_result == VICTOIRE_BLEU) {

            if (data->joueur == BLEU)
                texte = "VICTOIRE";
            else
                texte = "DEFAITE";

        }
        else if (data->game_result == VICTOIRE_ROUGE) {

            if (data->joueur == ROUGE)
                texte = "VICTOIRE";
            else
                texte = "DEFAITE";
        }
    }

    cairo_text_extents_t extents;

    cairo_text_extents(
        cr,
        texte,
        &extents
    );

    double tx =
        (width - extents.width) / 2.0
        - extents.x_bearing;

    double ty =
        (height - extents.height) / 2.0
        - extents.y_bearing;

    cairo_move_to(
        cr,
        tx,
        ty
    );

    cairo_show_text(
        cr,
        texte
    );
}


}

/* ============================================================

* IA CLIENT : BARRICADE BLEUE
* ============================================================ */

static void ia_poser_barricade_client(
InterfaceData *data
)
{
if (data == NULL)
return;


if (data->mode != MODE_CLIENT)
    return;

if (!data->ia_active)
    return;

if (data->game_result != PARTIE_EN_COURS)
    return;

if (jeu_joueur_actuel() != BLEU)
    return;

if (croix_posee_j1)
    return;


Coord libres[NB_CASES_BLEU];

int nb_libres = 0;


for (int i = 0;
     i < NB_CASES_BLEU;
     i++) {

    int row =
        CASES_AUTORISEES_BLEU[i].row;

    int col =
        CASES_AUTORISEES_BLEU[i].col;

    if (jeu_piece_at(
            row,
            col
        ) != -1)
        continue;

    if (jeu_est_barricade(
            row,
            col
        ))
        continue;

    libres[nb_libres].row = row;
    libres[nb_libres].col = col;

    nb_libres++;
}


if (nb_libres == 0)
    return;


int choix =
    rand() % nb_libres;

int row =
    libres[choix].row;

int col =
    libres[choix].col;


char message[BUFFER_SIZE];


snprintf(
    message,
    sizeof(message),
    "0.0,%d.%d",
    col,
    row
);


printf(
    "IA BLEU : %s\n",
    message
);


if (joueur_send_message(
        data->socket,
        message
    ) < 0) {

    printf(
        "IA BLEU : erreur lors de "
        "l'envoi de la barricade.\n"
    );
}


}

/* ============================================================

* IA CLIENT : DÉPLACEMENT BLEU
* ============================================================ */

static void ia_jouer_coup_client(
InterfaceData *data
)
{
if (data == NULL)
return;


if (data->mode != MODE_CLIENT)
    return;

if (!data->ia_active)
    return;

if (data->game_result != PARTIE_EN_COURS)
    return;

if (jeu_joueur_actuel() != BLEU)
    return;


int board[ROWS][COLS];


for (int row = 0;
     row < ROWS;
     row++) {

    for (int col = 0;
         col < COLS;
         col++) {

        board[row][col] =
            jeu_valeur_case(
                row,
                col
            );
    }
}


PawnMoveProposition coup =
    randomMove(
        board,
        BLEU
    );


int piece_index =
    jeu_piece_at(
        coup.startRow,
        coup.startCol
    );


if (piece_index == -1) {

    printf(
        "IA BLEU : aucune pièce trouvée.\n"
    );

    return;
}


Piece *piece =
    jeu_piece(piece_index);


if (piece == NULL ||
    piece->color != BLEU) {

    printf(
        "IA BLEU : pièce invalide.\n"
    );

    return;
}


int ligne_depart =
    piece->row;

int colonne_depart =
    piece->col;

int ligne_arrivee =
    coup.endRow;

int colonne_arrivee =
    coup.endCol;


if (!jeu_peut_deplacer(
        piece_index,
        ligne_arrivee,
        colonne_arrivee
    )) {

    printf(
        "IA BLEU : déplacement invalide.\n"
    );

    return;
}


char message[BUFFER_SIZE];


snprintf(
    message,
    sizeof(message),
    "%d.%d,%d.%d",
    colonne_depart,
    ligne_depart,
    colonne_arrivee,
    ligne_arrivee
);


printf(
    "IA BLEU : %s\n",
    message
);


if (joueur_send_message(
        data->socket,
        message
    ) < 0) {

    printf(
        "IA BLEU : erreur lors de "
        "l'envoi du déplacement.\n"
    );
}


}

/* ============================================================

* IA SERVEUR : BARRICADE ROUGE
* ============================================================ */

static void ia_poser_barricade_serveur(
InterfaceData *data
)
{
if (data == NULL)
return;


if (data->mode != MODE_SERVEUR)
    return;

if (!data->ia_active)
    return;

if (jeu_joueur_actuel() != ROUGE)
    return;

if (croix_posee_j2)
    return;


Coord libres[NB_CASES_ROUGE];

int nb_libres = 0;


for (int i = 0;
     i < NB_CASES_ROUGE;
     i++) {

    int row =
        CASES_AUTORISEES_ROUGE[i].row;

    int col =
        CASES_AUTORISEES_ROUGE[i].col;

    if (jeu_piece_at(
            row,
            col
        ) != -1)
        continue;

    if (jeu_est_barricade(
            row,
            col
        ))
        continue;

    libres[nb_libres].row = row;
    libres[nb_libres].col = col;

    nb_libres++;
}


if (nb_libres == 0)
    return;


int choix =
    rand() % nb_libres;

int row =
    libres[choix].row;

int col =
    libres[choix].col;


jeu_poser_barricade(
    row,
    col
);


x2_col = col;
x2_row = row;

croix_posee_j2 = true;


jeu_changer_joueur();


char message[BUFFER_SIZE];


snprintf(
    message,
    sizeof(message),
    "10.6,%d.%d",
    col,
    row
);


if (data->socket >= 0) {

    if (send_message(
            data->socket,
            message
        ) < 0) {

        printf(
            "Erreur lors de l'envoi "
            "de la barricade IA.\n"
        );
    }
}


printf(
    "IA ROUGE : %s\n",
    message
);


if (data->drawing_area != NULL) {

    gtk_widget_queue_draw(
        data->drawing_area
    );
}


}

/* ============================================================

* IA SERVEUR : DÉPLACEMENT ROUGE
* ============================================================ */

static void ia_jouer_coup_serveur(
InterfaceData *data
)
{
if (data == NULL)
return;


if (data->mode != MODE_SERVEUR)
    return;

if (!data->ia_active)
    return;

if (jeu_joueur_actuel() != ROUGE)
    return;

if (data->game_result != PARTIE_EN_COURS)
    return;


int board[ROWS][COLS];


for (int row = 0;
     row < ROWS;
     row++) {

    for (int col = 0;
         col < COLS;
         col++) {

        board[row][col] =
            jeu_valeur_case(
                row,
                col
            );
    }
}


PawnMoveProposition coup =
    randomMove(
        board,
        ROUGE
    );


int piece_index =
    jeu_piece_at(
        coup.startRow,
        coup.startCol
    );


if (piece_index == -1) {

    printf(
        "IA ROUGE : aucune pièce trouvée.\n"
    );

    return;
}


Piece *piece =
    jeu_piece(piece_index);


if (piece == NULL ||
    piece->color != ROUGE) {

    printf(
        "IA ROUGE : pièce invalide.\n"
    );

    return;
}


int ligne_depart =
    piece->row;

int colonne_depart =
    piece->col;

int ligne_arrivee =
    coup.endRow;

int colonne_arrivee =
    coup.endCol;


int joueur_avant_coup =
    jeu_joueur_actuel();


if (!jeu_deplacer(
        piece_index,
        ligne_arrivee,
        colonne_arrivee
    )) {

    printf(
        "IA ROUGE : déplacement invalide.\n"
    );

    return;
}


effectuer_captures(
    ligne_depart,
    colonne_depart,
    ligne_arrivee,
    colonne_arrivee,
    joueur_avant_coup
);


data->round++;


verifier_fin_partie(
    data
);


char message[BUFFER_SIZE];


if (data->game_result != PARTIE_EN_COURS) {

    snprintf(
        message,
        sizeof(message),
        "%d.%d,%d.%d|FIN:%d",
        colonne_depart,
        ligne_depart,
        colonne_arrivee,
        ligne_arrivee,
        data->game_result
    );

}
else {

    snprintf(
        message,
        sizeof(message),
        "%d.%d,%d.%d",
        colonne_depart,
        ligne_depart,
        colonne_arrivee,
        ligne_arrivee
    );
}


if (data->socket >= 0) {

    if (send_message(
            data->socket,
            message
        ) < 0) {

        printf(
            "Erreur lors de l'envoi "
            "du déplacement IA.\n"
        );
    }
}


printf(
    "IA ROUGE : %s\n",
    message
);


if (data->drawing_area != NULL) {

    gtk_widget_queue_draw(
        data->drawing_area
    );
}


}

/* ============================================================

* GESTIONNAIRE DE CLIC
* ============================================================ */

static void board_clicked(
GtkGestureClick *gesture,
int n_press,
double x,
double y,
gpointer user_data
)
{
(void)gesture;
(void)n_press;


InterfaceData *data =
    (InterfaceData *)user_data;


if (data == NULL)
    return;


if (data->game_result != PARTIE_EN_COURS)
    return;


GtkDrawingArea *drawing_area =
    GTK_DRAWING_AREA(
        data->drawing_area
    );


int width =
    gtk_widget_get_width(
        GTK_WIDGET(drawing_area)
    );

int height =
    gtk_widget_get_height(
        GTK_WIDGET(drawing_area)
    );


if (width <= 0 ||
    height <= 0)
    return;


double cell_w =
    (double)width / COLS;

double cell_h =
    (double)height / ROWS;


int col =
    (int)(x / cell_w);

int row =
    (int)(y / cell_h);


if (row < 0 ||
    row >= ROWS ||
    col < 0 ||
    col >= COLS)
    return;


int joueur =
    jeu_joueur_actuel();


if (data->mode == MODE_SERVEUR &&
    data->ia_active &&
    joueur == ROUGE) {

    return;
}


if (data->mode == MODE_CLIENT &&
    data->ia_active &&
    joueur == BLEU) {

    return;
}


if (data->mode != MODE_LOCAL &&
    joueur != data->joueur) {

    return;
}


/* ========================================================
 * PHASE 1 : BARRICADE BLEUE
 * ======================================================== */

if (joueur == BLEU &&
    !croix_posee_j1) {

    if (!est_dans_tableau(
            row,
            col,
            CASES_AUTORISEES_BLEU,
            NB_CASES_BLEU
        ))
        return;


    if (data->mode == MODE_CLIENT) {

        char message[BUFFER_SIZE];


        snprintf(
            message,
            sizeof(message),
            "0.0,%d.%d",
            col,
            row
        );


        printf(
            "%s\n",
            message
        );


        if (joueur_send_message(
                data->socket,
                message
            ) < 0) {

            printf(
                "Erreur lors de l'envoi "
                "de la barricade.\n"
            );
        }


        return;
    }


    x1_col = col;
    x1_row = row;


    croix_posee_j1 = true;


    jeu_poser_barricade(
        row,
        col
    );


    jeu_changer_joueur();


    if (data->mode == MODE_SERVEUR &&
        data->socket >= 0) {

        char message[BUFFER_SIZE];


        snprintf(
            message,
            sizeof(message),
            "0.0,%d.%d",
            col,
            row
        );


        if (send_message(
                data->socket,
                message
            ) < 0) {

            printf(
                "Erreur lors de l'envoi "
                "de la barricade.\n"
            );
        }


        printf(
            "%s\n",
            message
        );
    }


    gtk_widget_queue_draw(
        GTK_WIDGET(drawing_area)
    );


    if (data->mode == MODE_SERVEUR &&
        data->ia_active) {

        ia_poser_barricade_serveur(
            data
        );
    }


    return;
}


/* ========================================================
 * PHASE 1 : BARRICADE ROUGE
 * ======================================================== */

if (joueur == ROUGE &&
    !croix_posee_j2) {

    if (!est_dans_tableau(
            row,
            col,
            CASES_AUTORISEES_ROUGE,
            NB_CASES_ROUGE
        ))
        return;


    if (data->mode == MODE_LOCAL) {

        x2_col = col;
        x2_row = row;

        croix_posee_j2 = true;

        jeu_poser_barricade(
            row,
            col
        );

        jeu_changer_joueur();

        gtk_widget_queue_draw(
            GTK_WIDGET(drawing_area)
        );

        return;
    }


    if (data->mode == MODE_SERVEUR) {

        x2_col = col;
        x2_row = row;

        croix_posee_j2 = true;

        jeu_poser_barricade(
            row,
            col
        );

        jeu_changer_joueur();

        if (data->socket >= 0) {

            char message[BUFFER_SIZE];

            snprintf(
                message,
                sizeof(message),
                "10.6,%d.%d",
                col,
                row
            );

            if (send_message(
                    data->socket,
                    message
                ) < 0) {

                printf(
                    "Erreur lors de l'envoi "
                    "de la barricade.\n"
                );
            }

            printf(
                "%s\n",
                message
            );
        }

        gtk_widget_queue_draw(
            GTK_WIDGET(drawing_area)
        );

        return;
    }

    return;
}


/* ========================================================
 * PHASE 2 : SÉLECTION D'UNE PIÈCE
 * ======================================================== */

if (selected_piece == -1) {

    int p =
        jeu_piece_at(
            row,
            col
        );

    if (p != -1) {

        Piece *piece =
            jeu_piece(p);

        if (piece != NULL &&
            piece->color == joueur) {

            selected_piece = p;

            gtk_widget_queue_draw(
                GTK_WIDGET(drawing_area)
            );
        }
    }

    return;
}


/* ========================================================
 * PIÈCE SÉLECTIONNÉE
 * ======================================================== */

Piece *piece =
    jeu_piece(selected_piece);


if (piece == NULL) {

    selected_piece = -1;

    gtk_widget_queue_draw(
        GTK_WIDGET(drawing_area)
    );

    return;
}


if (row == piece->row &&
    col == piece->col) {

    selected_piece = -1;

    gtk_widget_queue_draw(
        GTK_WIDGET(drawing_area)
    );

    return;
}


int ligne_depart =
    piece->row;

int colonne_depart =
    piece->col;

int ligne_arrivee =
    row;

int colonne_arrivee =
    col;

int joueur_avant_coup =
    jeu_joueur_actuel();


/* ========================================================
 * MODE CLIENT
 * ======================================================== */

if (data->mode == MODE_CLIENT) {

    char message[BUFFER_SIZE];


    snprintf(
        message,
        sizeof(message),
        "%d.%d,%d.%d",
        colonne_depart,
        ligne_depart,
        colonne_arrivee,
        ligne_arrivee
    );


    printf(
        "%s\n",
        message
    );


    if (joueur_send_message(
            data->socket,
            message
        ) < 0) {

        printf(
            "Erreur lors de l'envoi "
            "du déplacement.\n"
        );
    }


    selected_piece = -1;


    gtk_widget_queue_draw(
        GTK_WIDGET(drawing_area)
    );


    return;
}


/* ========================================================
 * MODE LOCAL OU SERVEUR
 * ======================================================== */

if (jeu_deplacer(
        selected_piece,
        ligne_arrivee,
        colonne_arrivee
    )) {

    effectuer_captures(
        ligne_depart,
        colonne_depart,
        ligne_arrivee,
        colonne_arrivee,
        joueur_avant_coup
    );


    data->round++;


    verifier_fin_partie(
        data
    );


    if (data->mode == MODE_SERVEUR &&
        data->socket >= 0) {

        char message[BUFFER_SIZE];


        if (data->game_result != PARTIE_EN_COURS) {

            snprintf(
                message,
                sizeof(message),
                "%d.%d,%d.%d|FIN:%d",
                colonne_depart,
                ligne_depart,
                colonne_arrivee,
                ligne_arrivee,
                data->game_result
            );

        }
        else {

            snprintf(
                message,
                sizeof(message),
                "%d.%d,%d.%d",
                colonne_depart,
                ligne_depart,
                colonne_arrivee,
                ligne_arrivee
            );
        }


        if (send_message(
                data->socket,
                message
            ) < 0) {

            printf(
                "Erreur lors de l'envoi "
                "du déplacement.\n"
            );
        }


        printf(
            "%s\n",
            message
        );
    }


    selected_piece = -1;


    gtk_widget_queue_draw(
        GTK_WIDGET(drawing_area)
    );


    if (data->mode == MODE_SERVEUR &&
        data->ia_active &&
        data->game_result == PARTIE_EN_COURS &&
        jeu_joueur_actuel() == ROUGE) {

        ia_jouer_coup_serveur(
            data
        );
    }
}


}

/* ============================================================

* RÉSEAU : CLIENT
* ============================================================ */

static gboolean network_callback(
gint fd,
GIOCondition condition,
gpointer user_data
)
{
InterfaceData *data =
(InterfaceData *)user_data;


if (condition & (
        G_IO_HUP |
        G_IO_ERR |
        G_IO_NVAL
    )) {

    printf(
        "Connexion au serveur perdue.\n"
    );

    return G_SOURCE_REMOVE;
}


if (!(condition & G_IO_IN))
    return G_SOURCE_CONTINUE;


char buffer[BUFFER_SIZE];


int result =
    joueur_receive_message(
        fd,
        buffer,
        sizeof(buffer)
    );


if (result <= 0) {

    printf(
        "Connexion au serveur perdue.\n"
    );

    return G_SOURCE_REMOVE;
}


printf(
    "Serveur : %s\n",
    buffer
);


/* ========================================================
 * BARRICADE BLEUE
 *
 * IMPORTANT :
 * on ne reconnaît 0.0,... comme barricade que
 * pendant la phase de pose.
 * ======================================================== */

int x1;
int y1;
int x2;
int y2;


if (!croix_posee_j1 &&
    sscanf(
        buffer,
        "0.0,%d.%d",
        &x2,
        &y2
    ) == 2) {

    int row = y2;
    int col = x2;


    if (row < 0 ||
        row >= ROWS ||
        col < 0 ||
        col >= COLS) {

        printf(
            "Barricade reçue invalide.\n"
        );

        return G_SOURCE_CONTINUE;
    }


    jeu_poser_barricade(
        row,
        col
    );


    x1_col = col;
    x1_row = row;


    croix_posee_j1 = true;


    jeu_changer_joueur();


    if (data->drawing_area != NULL) {

        gtk_widget_queue_draw(
            data->drawing_area
        );
    }


    return G_SOURCE_CONTINUE;
}


/* ========================================================
 * BARRICADE ROUGE
 *
 * Même principe : 10.6,... n'est une barricade
 * que pendant la phase de pose.
 * ======================================================== */

if (!croix_posee_j2 &&
    sscanf(
        buffer,
        "10.6,%d.%d",
        &x2,
        &y2
    ) == 2) {

    int row = y2;
    int col = x2;


    if (row < 0 ||
        row >= ROWS ||
        col < 0 ||
        col >= COLS) {

        printf(
            "Barricade reçue invalide.\n"
        );

        return G_SOURCE_CONTINUE;
    }


    jeu_poser_barricade(
        row,
        col
    );


    x2_col = col;
    x2_row = row;


    croix_posee_j2 = true;


    jeu_changer_joueur();


    if (data->drawing_area != NULL) {

        gtk_widget_queue_draw(
            data->drawing_area
        );
    }


    if (data->mode == MODE_CLIENT &&
        data->ia_active &&
        data->game_result == PARTIE_EN_COURS &&
        jeu_joueur_actuel() == BLEU) {

        ia_jouer_coup_client(
            data
        );
    }


    return G_SOURCE_CONTINUE;
}


/* ========================================================
 * DÉPLACEMENT
 * ======================================================== */

if (sscanf(
        buffer,
        "%d.%d,%d.%d",
        &x1,
        &y1,
        &x2,
        &y2
    ) == 4) {

    int colonne_depart = x1;
    int ligne_depart = y1;

    int colonne_arrivee = x2;
    int ligne_arrivee = y2;


    if (ligne_depart < 0 ||
        ligne_depart >= ROWS ||
        colonne_depart < 0 ||
        colonne_depart >= COLS ||
        ligne_arrivee < 0 ||
        ligne_arrivee >= ROWS ||
        colonne_arrivee < 0 ||
        colonne_arrivee >= COLS) {

        printf(
            "Déplacement reçu hors plateau.\n"
        );

        return G_SOURCE_CONTINUE;
    }


    int piece_index =
        jeu_piece_at(
            ligne_depart,
            colonne_depart
        );


    if (piece_index == -1) {

        printf(
            "Erreur : pièce introuvable "
            "à la position reçue.\n"
        );

        return G_SOURCE_CONTINUE;
    }


    Piece *piece_recue =
        jeu_piece(piece_index);


    if (piece_recue == NULL) {

        printf(
            "Erreur : pièce reçue invalide.\n"
        );

        return G_SOURCE_CONTINUE;
    }


    int joueur_avant_coup =
        piece_recue->color;


    if (!jeu_deplacer(
            piece_index,
            ligne_arrivee,
            colonne_arrivee
        )) {

        printf(
            "Déplacement reçu invalide.\n"
        );

        return G_SOURCE_CONTINUE;
    }


    effectuer_captures(
        ligne_depart,
        colonne_depart,
        ligne_arrivee,
        colonne_arrivee,
        joueur_avant_coup
    );


    data->round++;


    verifier_fin_partie(
        data
    );


    char *fin =
        strstr(
            buffer,
            "|FIN:"
        );


    if (fin != NULL) {

        int resultat_final;


        if (sscanf(
                fin,
                "|FIN:%d",
                &resultat_final
            ) == 1) {

            data->game_result =
                resultat_final;


            printf(
                "Résultat final reçu du serveur : %d\n",
                data->game_result
            );
        }
    }


    selected_piece = -1;


    if (data->drawing_area != NULL) {

        gtk_widget_queue_draw(
            data->drawing_area
        );
    }


    if (data->mode == MODE_CLIENT &&
        data->ia_active &&
        data->game_result == PARTIE_EN_COURS &&
        joueur_avant_coup == ROUGE &&
        jeu_joueur_actuel() == BLEU) {

        ia_jouer_coup_client(
            data
        );
    }
}


return G_SOURCE_CONTINUE;


}

/* ============================================================

* ACCEPTATION DU JOUEUR
* ============================================================ */

static gboolean server_accept_callback(
gint fd,
GIOCondition condition,
gpointer user_data
)
{
InterfaceData *data =
(InterfaceData *)user_data;


if (condition & (
        G_IO_HUP |
        G_IO_ERR |
        G_IO_NVAL
    )) {

    printf(
        "Erreur sur la socket du serveur.\n"
    );

    return G_SOURCE_REMOVE;
}


if (condition & G_IO_IN) {

    int client_socket =
        server_accept_player(fd);


    if (client_socket == -1)
        return G_SOURCE_CONTINUE;


    data->socket =
        client_socket;


    printf(
        "Joueur 1 connecté.\n"
    );


    g_unix_fd_add(
        client_socket,
        G_IO_IN |
        G_IO_HUP |
        G_IO_ERR |
        G_IO_NVAL,
        server_network_callback,
        data
    );


    return G_SOURCE_REMOVE;
}


return G_SOURCE_CONTINUE;


}

/* ============================================================

* RÉSEAU : SERVEUR
* ============================================================ */

static gboolean server_network_callback(
gint fd,
GIOCondition condition,
gpointer user_data
)
{
InterfaceData *data =
(InterfaceData *)user_data;


if (condition & (
        G_IO_HUP |
        G_IO_ERR |
        G_IO_NVAL
    )) {

    printf(
        "Connexion au joueur perdue.\n"
    );

    return G_SOURCE_REMOVE;
}


if (!(condition & G_IO_IN))
    return G_SOURCE_CONTINUE;


char buffer[BUFFER_SIZE];


int result =
    receive_message(
        fd,
        buffer,
        sizeof(buffer)
    );


if (result <= 0) {

    printf(
        "Connexion au joueur perdue.\n"
    );

    return G_SOURCE_REMOVE;
}


printf(
    "Joueur 1 : %s\n",
    buffer
);


/* ========================================================
 * BARRICADE BLEUE
 *
 * IMPORTANT :
 * la chaîne 0.0,... n'est interprétée comme
 * barricade que si la barricade n'a pas encore
 * été posée.
 * ======================================================== */

int x1;
int y1;
int x2;
int y2;


if (!croix_posee_j1 &&
    sscanf(
        buffer,
        "0.0,%d.%d",
        &x2,
        &y2
    ) == 2) {

    int row = y2;
    int col = x2;


    if (jeu_joueur_actuel() != BLEU) {

        printf(
            "Barricade refusée : "
            "ce n'est pas le tour du "
            "joueur BLEU.\n"
        );

        return G_SOURCE_CONTINUE;
    }


    if (!est_dans_tableau(
            row,
            col,
            CASES_AUTORISEES_BLEU,
            NB_CASES_BLEU
        )) {

        printf(
            "Barricade refusée : "
            "case interdite.\n"
        );

        return G_SOURCE_CONTINUE;
    }


    if (jeu_piece_at(
            row,
            col
        ) != -1 ||
        jeu_est_barricade(
            row,
            col
        )) {

        printf(
            "Barricade refusée : "
            "case occupée.\n"
        );

        return G_SOURCE_CONTINUE;
    }


    jeu_poser_barricade(
        row,
        col
    );


    x1_col = col;
    x1_row = row;


    croix_posee_j1 = true;


    jeu_changer_joueur();


    if (send_message(
            fd,
            buffer
        ) < 0) {

        printf(
            "Erreur lors de l'envoi "
            "de la barricade.\n"
        );

        return G_SOURCE_REMOVE;
    }


    printf(
        "%s\n",
        buffer
    );


    if (data->drawing_area != NULL) {

        gtk_widget_queue_draw(
            data->drawing_area
        );
    }


    if (data->ia_active &&
        data->game_result == PARTIE_EN_COURS) {

        ia_poser_barricade_serveur(
            data
        );
    }


    return G_SOURCE_CONTINUE;
}


/* ========================================================
 * DÉPLACEMENT BLEU
 * ======================================================== */

if (sscanf(
        buffer,
        "%d.%d,%d.%d",
        &x1,
        &y1,
        &x2,
        &y2
    ) == 4) {

    int colonne_depart = x1;
    int ligne_depart = y1;

    int colonne_arrivee = x2;
    int ligne_arrivee = y2;


    if (colonne_depart < 0 ||
        colonne_depart >= COLS ||
        ligne_depart < 0 ||
        ligne_depart >= ROWS ||
        colonne_arrivee < 0 ||
        colonne_arrivee >= COLS ||
        ligne_arrivee < 0 ||
        ligne_arrivee >= ROWS) {

        printf(
            "Déplacement refusé : "
            "coordonnées hors plateau.\n"
        );

        return G_SOURCE_CONTINUE;
    }


    if (jeu_joueur_actuel() != BLEU) {

        printf(
            "Déplacement refusé : "
            "ce n'est pas le tour "
            "du joueur BLEU.\n"
        );

        return G_SOURCE_CONTINUE;
    }


    int piece_index =
        jeu_piece_at(
            ligne_depart,
            colonne_depart
        );


    if (piece_index == -1) {

        printf(
            "Déplacement refusé : "
            "pièce introuvable.\n"
        );

        return G_SOURCE_CONTINUE;
    }


    Piece *piece =
        jeu_piece(piece_index);


    if (piece == NULL ||
        piece->color != BLEU) {

        printf(
            "Déplacement refusé : "
            "la pièce n'appartient "
            "pas au joueur BLEU.\n"
        );

        return G_SOURCE_CONTINUE;
    }


    int joueur_avant_coup =
        jeu_joueur_actuel();


    if (!jeu_deplacer(
            piece_index,
            ligne_arrivee,
            colonne_arrivee
        )) {

        printf(
            "Déplacement refusé : "
            "coup illégal.\n"
        );

        return G_SOURCE_CONTINUE;
    }


    effectuer_captures(
        ligne_depart,
        colonne_depart,
        ligne_arrivee,
        colonne_arrivee,
        joueur_avant_coup
    );


    data->round++;


    verifier_fin_partie(
        data
    );


    char message[BUFFER_SIZE];


    if (data->game_result != PARTIE_EN_COURS) {

        snprintf(
            message,
            sizeof(message),
            "%d.%d,%d.%d|FIN:%d",
            colonne_depart,
            ligne_depart,
            colonne_arrivee,
            ligne_arrivee,
            data->game_result
        );

    }
    else {

        snprintf(
            message,
            sizeof(message),
            "%d.%d,%d.%d",
            colonne_depart,
            ligne_depart,
            colonne_arrivee,
            ligne_arrivee
        );
    }


    if (send_message(
            fd,
            message
        ) < 0) {

        printf(
            "Erreur lors de l'envoi "
            "du déplacement.\n"
        );

        return G_SOURCE_REMOVE;
    }


    printf(
        "%s\n",
        message
    );


    selected_piece = -1;


    if (data->drawing_area != NULL) {

        gtk_widget_queue_draw(
            data->drawing_area
        );
    }


    if (data->ia_active &&
        data->game_result == PARTIE_EN_COURS &&
        jeu_joueur_actuel() == ROUGE) {

        ia_jouer_coup_serveur(
            data
        );
    }
}


return G_SOURCE_CONTINUE;


}

/* ============================================================

* CRÉATION DE LA FENÊTRE
* ============================================================ */

static void activate(
GtkApplication *app,
gpointer user_data
)
{
InterfaceData *data =
(InterfaceData *)user_data;
GtkWidget *window;
GtkWidget *drawing_area;


window =
    gtk_application_window_new(
        app
    );


/* ========================================================
 * TITRE
 * ======================================================== */

if (data->mode == MODE_SERVEUR) {

    if (data->ia_active) {

        gtk_window_set_title(
            GTK_WINDOW(window),
            "Klosnowo - Joueur 2 - IA"
        );

    } else {

        gtk_window_set_title(
            GTK_WINDOW(window),
            "Klosnowo - Joueur 2"
        );
    }

}
else if (data->mode == MODE_CLIENT) {

    if (data->ia_active) {

        gtk_window_set_title(
            GTK_WINDOW(window),
            "Klosnowo - Joueur 1 - IA"
        );

    } else {

        gtk_window_set_title(
            GTK_WINDOW(window),
            "Klosnowo - Joueur 1"
        );
    }

}
else {

    gtk_window_set_title(
        GTK_WINDOW(window),
        "Klosnowo - Partie locale"
    );
}


gtk_window_set_default_size(
    GTK_WINDOW(window),
    800,
    500
);


/* ========================================================
 * DRAWING AREA
 * ======================================================== */

drawing_area =
    gtk_drawing_area_new();


data->drawing_area =
    drawing_area;


gtk_drawing_area_set_content_width(
    GTK_DRAWING_AREA(drawing_area),
    800
);


gtk_drawing_area_set_content_height(
    GTK_DRAWING_AREA(drawing_area),
    500
);


gtk_drawing_area_set_draw_func(
    GTK_DRAWING_AREA(drawing_area),
    draw_board,
    data,
    NULL
);


/* ========================================================
 * CLIC
 * ======================================================== */

GtkGesture *click =
    gtk_gesture_click_new();


gtk_gesture_single_set_button(
    GTK_GESTURE_SINGLE(click),
    GDK_BUTTON_PRIMARY
);


g_signal_connect(
    click,
    "pressed",
    G_CALLBACK(board_clicked),
    data
);


gtk_widget_add_controller(
    drawing_area,
    GTK_EVENT_CONTROLLER(click)
);


gtk_window_set_child(
    GTK_WINDOW(window),
    drawing_area
);


gtk_window_present(
    GTK_WINDOW(window)
);


/* ========================================================
 * PREMIER COUP DE L'IA BLEUE
 * ======================================================== */

if (data->mode == MODE_CLIENT &&
    data->ia_active &&
    jeu_joueur_actuel() == BLEU) {

    if (!croix_posee_j1) {

        ia_poser_barricade_client(
            data
        );

    }
    else {

        ia_jouer_coup_client(
            data
        );
    }
}


}

/* ============================================================

* LANCEMENT CLIENT
* ============================================================ */

int interface_lancer(
int argc,
char **argv,
int socket,
int ia_active
)
{
(void)argc;


GtkApplication *app;
int status;


InterfaceData data;


data.mode =
    MODE_CLIENT;

data.joueur =
    BLEU;

data.socket =
    socket;

data.server_socket =
    -1;

data.drawing_area =
    NULL;

data.round =
    0;

data.game_result =
    PARTIE_EN_COURS;

data.ia_active =
    ia_active;


jeu_initialiser();


g_unix_fd_add(
    socket,
    G_IO_IN |
    G_IO_HUP |
    G_IO_ERR |
    G_IO_NVAL,
    network_callback,
    &data
);


app =
    gtk_application_new(
        "com.example.klosnowo",
        G_APPLICATION_DEFAULT_FLAGS
    );


g_signal_connect(
    app,
    "activate",
    G_CALLBACK(activate),
    &data
);


char *gtk_argv[] = {
    argv[0],
    NULL
};


status =
    g_application_run(
        G_APPLICATION(app),
        1,
        gtk_argv
    );


g_object_unref(app);


return status;


}

/* ============================================================

* LANCEMENT SERVEUR
* ============================================================ */

int interface_lancer_serveur(
int server_socket,
int ia_active
)
{
GtkApplication *app;
int status;


InterfaceData data;


data.mode =
    MODE_SERVEUR;

data.joueur =
    ROUGE;

data.socket =
    -1;

data.server_socket =
    server_socket;

data.drawing_area =
    NULL;

data.round =
    0;

data.game_result =
    PARTIE_EN_COURS;

data.ia_active =
    ia_active;


jeu_initialiser();


g_unix_fd_add(
    server_socket,
    G_IO_IN |
    G_IO_HUP |
    G_IO_ERR |
    G_IO_NVAL,
    server_accept_callback,
    &data
);


app =
    gtk_application_new(
        "com.example.klosnowo.server",
        G_APPLICATION_DEFAULT_FLAGS
    );


g_signal_connect(
    app,
    "activate",
    G_CALLBACK(activate),
    &data
);


char *gtk_argv[] = {
    (char *)"game",
    NULL
};


status =
    g_application_run(
        G_APPLICATION(app),
        1,
        gtk_argv
    );


if (data.socket >= 0)
    close(data.socket);


g_object_unref(app);


return status;


}

/* ============================================================

* LANCEMENT LOCAL
* ============================================================ */

int interface_lancer_local(void)
{
GtkApplication *app;
int status;


InterfaceData data;


data.mode =
    MODE_LOCAL;

data.joueur =
    BLEU;

data.socket =
    -1;

data.server_socket =
    -1;

data.drawing_area =
    NULL;

data.round =
    0;

data.game_result =
    PARTIE_EN_COURS;

data.ia_active =
    false;


jeu_initialiser();


app =
    gtk_application_new(
        "com.example.klosnowo.local",
        G_APPLICATION_DEFAULT_FLAGS
    );


g_signal_connect(
    app,
    "activate",
    G_CALLBACK(activate),
    &data
);


char *gtk_argv[] = {
    (char *)"game",
    NULL
};


status =
    g_application_run(
        G_APPLICATION(app),
        1,
        gtk_argv
    );


g_object_unref(app);


return status;
}
