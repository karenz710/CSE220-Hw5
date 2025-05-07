#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include "poker_client.h"
#include "client_action_handler.h"
#include "game_logic.h"

#define BASE_PORT 2201
#define NUM_PORTS 6
#define BUFFER_SIZE 1024

typedef struct {
    int socket;
    struct sockaddr_in address;
} player_t;

game_state_t game; //global variable to store our game state info (this is a huge hint for you)

int main(int argc, char **argv) {
    int server_fds[NUM_PORTS], client_socket, player_count = 0;
    int opt = 1;
    int listen_fd, conn_fd;
    struct sockaddr_in server_address;
    player_t players[MAX_PLAYERS];
    char buffer[BUFFER_SIZE] = {0};
    socklen_t addrlen = sizeof(struct sockaddr_in);

    //Setup the server infrastructre and accept the 6 players on ports 2201, 2202, 2203, 2204, 2205, 2206
    // create NUM_PORTS sockets 
    for (int i = 0; i < NUM_PORTS; i++) {
        if ((listen_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
            perror("socket failed");
            exit(EXIT_FAILURE);
        }

        // set socket options
        if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
            perror("setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))");
            exit(EXIT_FAILURE);
        }

        // Bind socket to each 6 port (associate server's socket address in server_address with the socket descriptor listen_fd)
        server_address.sin_family = AF_INET; // protocol family using IPv4 addresses
        server_address.sin_addr.s_addr = INADDR_ANY;
        // htons convert host to network short - convert host byte order to network byte order
        server_address.sin_port = htons(BASE_PORT + i); // Port num in network byte order (big endian)
        if (bind(listen_fd, (struct sockaddr *)&server_address, addrlen) < 0) {
            perror("[Server] bind() failed.");
            exit(EXIT_FAILURE);
        }

        // listen for incoming connections (passively wait for connection requests from clients)
        if (listen(listen_fd, 1) < 0) {
            perror("[Server] listen() failed");
            exit(EXIT_FAILURE);
        }
        server_fds[i] = listen_fd;
    }

    int rand_seed = argc == 2 ? atoi(argv[1]) : 0;
    init_game_state(&game, 100, rand_seed);

    //Join state?
    // accept incoming connections
    for (int i = 0; i < NUM_PORTS; i++) {
        struct sockaddr_in client_addr;
        if ((conn_fd = accept(listen_fd, (struct sockaddr *)&client_addr, (socklen_t *)&addrlen)) < 0) {
            perror("[Server] accept() failed.");
            exit(EXIT_FAILURE);
        }
        players[i].address = client_addr;
        players[i].socket = conn_fd;
        player_count++;
    }
    
    while (1) {
       
        // READY
        
        // DEAL TO PLAYERS

        // PREFLOP BETTING

        // PLACE FLOP CARDS

        // FLOP BETTING

        // PLACE TURN CARDS

        // TURN BETTING

        // PLACE RIVER CARDS

        // RIVER BETTING
        
        // ROUND_SHOWDOWN
    }

    printf("[Server] Shutting down.\n");

    // Close all fds (you're welcome)
    for (int i = 0; i < MAX_PLAYERS; i++) {
        close(server_fds[i]);
        if (game.player_status[i] != PLAYER_LEFT) {
            close(game.sockets[i]);
        }
    }

    return 0;
}