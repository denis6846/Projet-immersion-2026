#include "network/joueur.h"
#include "network/protocole.h"

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stddef.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int joueur_connect(const char *address, int port)
{
    int joueur_socket;
    struct sockaddr_in server_addr;

    /* Création de la socket */
    joueur_socket = socket(AF_INET, SOCK_STREAM, 0);

    if (joueur_socket == -1) {
        perror("socket");
        return -1;
    }

    /* Configuration de l'adresse du serveur */
    memset(&server_addr, 0, sizeof(server_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    if (inet_pton(AF_INET,
                  address,
                  &server_addr.sin_addr) <= 0) {

        perror("inet_pton");
        close(joueur_socket);
        return -1;
    }

    /* Connexion au serveur */
    if (connect(joueur_socket,
                (struct sockaddr *)&server_addr,
                sizeof(server_addr)) == -1) {

        perror("connect");
        close(joueur_socket);
        return -1;
    }

    printf("Connexion au serveur réussie.\n");

    return joueur_socket;
}

void joueur_disconnect(int joueur_socket)
{
    if (joueur_socket >= 0) {
        close(joueur_socket);
    }
}

int joueur_send_message(int joueur_socket, const char *message)
{
    return send_message(joueur_socket, message);
}

int joueur_receive_message(int joueur_socket,
                           char *buffer,
                           size_t buffer_size)
{
    int result = receive_message(joueur_socket, buffer, buffer_size);

    if (result > 0) {
        printf("Message reçu : %s\n", buffer);
    }

    return result;
}

int joueur_envoyer_coup(
    int joueur_socket,
    int x1,
    int y1,
    int x2,
    int y2)
{
    char message[BUFFER_SIZE];

    snprintf(
        message,
        sizeof(message),
        "%d.%d,%d.%d",
        x1,
        y1,
        x2,
        y2
    );

    return joueur_send_message(
        joueur_socket,
        message
    );
}