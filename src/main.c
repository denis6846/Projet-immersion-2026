#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "GUI/interface.h"
#include "network/server.h"
#include "network/joueur.h"
#include "jeu/checkScore.h"

int main(int argc, char *argv[])
{
    /*
     * Vérification générale des arguments.
     */
    if (argc < 2) {
        printf("Usage :\n");
        printf("  %s -s [-ia] <port>\n", argv[0]);
        printf("  %s -c [-ia] <adresse>:<port>\n", argv[0]);
        printf("  %s -l\n", argv[0]);
        return EXIT_FAILURE;
    }

    /*
     * ============================================================
     * SERVEUR
     * ============================================================
     *
     * Le serveur est le joueur 2 (ROUGE).
     *
     * Possibilités :
     *   ./game -s 5000
     *   ./game -s -ia 5000
     */

    if (strcmp(argv[1], "-s") == 0) {

        int ia_active = 0;
        int port_index = 2;

        /*
         * Vérification de l'option -ia.
         */
        if (argc == 4) {

            if (strcmp(argv[2], "-ia") == 0) {
                ia_active = 1;
                port_index = 3;
            }
            else {
                printf("Option inconnue : %s\n", argv[2]);
                printf(
                    "Usage : %s -s [-ia] <port>\n",
                    argv[0]
                );
                return EXIT_FAILURE;
            }
        }
        else if (argc != 3) {

            printf(
                "Usage : %s -s [-ia] <port>\n",
                argv[0]
            );
            return EXIT_FAILURE;
        }

        /*
         * Récupération du port.
         */
        int port = atoi(argv[port_index]);

        if (port < 1 || port > 65535) {
            printf("Port invalide.\n");
            return EXIT_FAILURE;
        }

        /*
         * Démarrage du serveur.
         */
        if (ia_active) {
            printf("Démarrage du serveur avec IA...\n");
        }
        else {
            printf("Démarrage du serveur...\n");
        }

        int server_socket = server_start(port);

        if (server_socket == -1) {
            printf("Impossible de démarrer le serveur.\n");
            return EXIT_FAILURE;
        }

        /*
         * Le serveur représente le joueur 2 (ROUGE).
         */
        int status = interface_lancer_serveur(
            server_socket,
            ia_active
        );

        server_stop(server_socket);

        return status;
    }

    /*
     * ============================================================
     * CLIENT
     * ============================================================
     *
     * Le client est le joueur 1 (BLEU).
     *
     * Possibilités :
     *   ./game -c 127.0.0.1:5000
     *   ./game -c -ia 127.0.0.1:5000
     *
     * Avec -ia, le joueur BLEU est contrôlé par l'IA.
     */

    if (strcmp(argv[1], "-c") == 0) {

        int ia_active = 0;
        int address_index = 2;

        /*
         * Vérification de l'option -ia.
         */
        if (argc == 4) {

            if (strcmp(argv[2], "-ia") == 0) {
                ia_active = 1;
                address_index = 3;
            }
            else {
                printf("Option inconnue : %s\n", argv[2]);
                printf(
                    "Usage : %s -c [-ia] <adresse>:<port>\n",
                    argv[0]
                );
                return EXIT_FAILURE;
            }
        }
        else if (argc != 3) {

            printf(
                "Usage : %s -c [-ia] <adresse>:<port>\n",
                argv[0]
            );
            return EXIT_FAILURE;
        }

        /*
         * Récupération de l'adresse et du port.
         */
        char address[256];
        int port;

        if (sscanf(
                argv[address_index],
                "%255[^:]:%d",
                address,
                &port
            ) != 2) {

            printf(
                "Format invalide. Utilisez : "
                "%s -c [-ia] <adresse>:<port>\n",
                argv[0]
            );

            return EXIT_FAILURE;
        }

        if (port < 1 || port > 65535) {
            printf("Port invalide.\n");
            return EXIT_FAILURE;
        }

        /*
         * Connexion au serveur.
         */
        if (ia_active) {
            printf(
                "Connexion au serveur %s:%d avec IA...\n",
                address,
                port
            );
        }
        else {
            printf(
                "Connexion au serveur %s:%d...\n",
                address,
                port
            );
        }

        int socket = joueur_connect(address, port);

        if (socket == -1) {
            printf(
                "Impossible de se connecter au serveur.\n"
            );
            return EXIT_FAILURE;
        }

        printf("Connexion au serveur réussie.\n");

        /*
         * Le client représente le joueur 1 (BLEU).
         *
         * ia_active indique si BLEU doit être contrôlé
         * par l'IA.
         */
        int status = interface_lancer(
            argc,
            argv,
            socket,
            ia_active
        );

        joueur_disconnect(socket);

        return status;
    }

    /*
     * ============================================================
     * PARTIE LOCALE
     * ============================================================
     */

    if (strcmp(argv[1], "-l") == 0) {

        if (argc != 2) {
            printf("Usage : %s -l\n", argv[0]);
            return EXIT_FAILURE;
        }

        /*
         * Lancement de la partie locale.
         */
        int status = interface_lancer_local();

        /*
         * TEST TEMPORAIRE DU SCORE
         *
         * À retirer une fois le système de score
         * intégré directement dans l'interface.
         */
        result score = giveScore();

        printf("\n");
        printf("===== TEST SCORE =====\n");
        printf("Score BLEU  : %d\n", score.blueScore);
        printf("Score ROUGE : %d\n", score.redScore);
        printf("======================\n");

        return status;
    }

    /*
     * ============================================================
     * OPTION INCONNUE
     * ============================================================
     */

    printf("Option inconnue : %s\n", argv[1]);

    printf("Usage :\n");
    printf("  %s -s [-ia] <port>\n", argv[0]);
    printf("  %s -c [-ia] <adresse>:<port>\n", argv[0]);
    printf("  %s -l\n", argv[0]);

    return EXIT_FAILURE;
}