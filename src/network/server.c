#include "network/server.h"
#include "network/protocole.h"
#include "jeu/jeu.h"
#include "jeu/capture.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

int server_start(int port)
{
    int server_socket;
    int option = 1;

    struct sockaddr_in server_addr;

    /*
     * Création de la socket TCP
     */
    server_socket = socket(AF_INET, SOCK_STREAM, 0);

    if (server_socket == -1) {
        perror("socket");
        return -1;
    }

    /*
     * Réutilisation immédiate du port
     */
    if (setsockopt(
            server_socket,
            SOL_SOCKET,
            SO_REUSEADDR,
            &option,
            sizeof(option)) == -1) {

        perror("setsockopt");
        close(server_socket);
        return -1;
    }

    /*
     * Configuration de l'adresse
     */
    memset(&server_addr, 0, sizeof(server_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    /*
     * Association IP / port
     */
    if (bind(
            server_socket,
            (struct sockaddr *)&server_addr,
            sizeof(server_addr)) == -1) {

        perror("bind");
        close(server_socket);
        return -1;
    }

    /*
     * Un seul client réseau :
     * le joueur 1.
     *
     * Le joueur 2 est le serveur lui-même.
     */
    if (listen(server_socket, 1) == -1) {

        perror("listen");
        close(server_socket);
        return -1;
    }

    printf("Serveur en écoute sur le port %d...\n", port);
    printf("Le serveur joue en tant que joueur 2.\n");
    printf("En attente du joueur 1...\n");

    /*
     * On retourne immédiatement la socket d'écoute.
     *
     * L'interface GTK pourra donc être lancée tout de suite,
     * sans bloquer sur accept().
     */
    return server_socket;
}

int server_accept_player(int server_socket)
{
    int player_socket;

    player_socket = accept(
        server_socket,
        NULL,
        NULL
    );

    if (player_socket == -1) {
        perror("accept");
        return -1;
    }

    printf("Joueur 1 connecté.\n");

    /*
     * Message initial envoyé au client.
     */
    if (send_message(
            player_socket,
            "Bienvenue joueur 1 !") < 0) {

        perror("send joueur 1");
        close(player_socket);
        return -1;
    }

    return player_socket;
}

void server_stop(int server_socket)
{
    if (server_socket >= 0) {
        close(server_socket);
    }
}

int server_traiter_coup(const char *message)
{
    int x1, y1, x2, y2;
    int piece_index;
    int joueur;
    int captures;

    if (sscanf(message, "%d.%d,%d.%d",
               &x1, &y1, &x2, &y2) != 4) {
        printf("Coup invalide : %s\n", message);
        return 0;
    }

    /* Vérification des coordonnées */
    if (x1 < 0 || x1 >= COLS ||
        x2 < 0 || x2 >= COLS ||
        y1 < 0 || y1 >= ROWS ||
        y2 < 0 || y2 >= ROWS) {
        printf("Coordonnées invalides.\n");
        return 0;
    }

    /*
     * Même case = pose d'une barricade
     */
    if (x1 == x2 && y1 == y2) {

        if (jeu_piece_at(y1, x1) != -1) {
            printf("Impossible de poser une barricade sur une pièce.\n");
            return 0;
        }

        if (jeu_est_barricade(y1, x1)) {
            printf("Une barricade existe déjà ici.\n");
            return 0;
        }

        jeu_poser_barricade(y1, x1);
        jeu_changer_joueur();

        printf("Barricade posée en (%d,%d).\n", x1, y1);

        return 1;
    }

    /*
     * Sinon, c'est un déplacement de pièce.
     */

    /* On mémorise le joueur AVANT le déplacement */
    joueur = jeu_joueur_actuel();

    piece_index = jeu_piece_at(y1, x1);

    if (piece_index == -1) {
        printf("Aucune pièce en (%d,%d).\n", x1, y1);
        return 0;
    }

    if (jeu_deplacer(piece_index, y2, x2) == 0) {
        printf("Déplacement refusé par les règles.\n");
        return 0;
    }

    printf("Déplacement : %d.%d -> %d.%d\n",
           x1, y1, x2, y2);

    /*
     * Vérification des captures après le déplacement.
     */
    captures = effectuer_captures(
        y1, x1,
        y2, x2,
        joueur
    );

    printf("Captures effectuées : %d\n", captures);

    return 1;
}