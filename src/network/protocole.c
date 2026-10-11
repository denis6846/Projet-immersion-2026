#include "network/protocole.h"

#include <sys/socket.h>
#include <string.h>
#include <stdio.h>

int send_message(int socket, const char *message)
{
    char buffer[BUFFER_SIZE];

    int length = snprintf(
        buffer,
        sizeof(buffer),
        "%s\n",
        message
    );

    if (length < 0 || length >= BUFFER_SIZE)
        return -1;

    return send(
        socket,
        buffer,
        length,
        0
    );
}

int receive_message(
    int socket,
    char *buffer,
    int buffer_size)
{
    int received = 0;
    char character;

    if (buffer == NULL || buffer_size <= 1)
        return -1;

    while (received < buffer_size - 1)
    {
        int result = recv(
            socket,
            &character,
            1,
            0
        );

        if (result <= 0)
            return result;

        if (character == '\n')
            break;

        buffer[received] = character;
        received++;
    }

    buffer[received] = '\0';

    return received;
}