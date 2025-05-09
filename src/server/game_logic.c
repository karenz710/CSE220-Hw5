#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>

#include "poker_client.h"
#include "client_action_handler.h"
#include "game_logic.h"

//Feel free to add your own code. I stripped out most of our solution functions but I left some "breadcrumbs" for anyone lost

// for debugging
void print_game_state( game_state_t *game){
    for (int i = 0; i < MAX_PLAYERS; i++) {
        printf("player status %d is %d\n", i, game->player_status[i]);
    }
}

void init_deck(card_t deck[DECK_SIZE], int seed){ //DO NOT TOUCH THIS FUNCTION
    srand(seed);
    int i = 0;
    for(int r = 0; r<13; r++){
        for(int s = 0; s<4; s++){
            deck[i++] = (r << SUITE_BITS) | s;
        }
    }
}

void shuffle_deck(card_t deck[DECK_SIZE]){ //DO NOT TOUCH THIS FUNCTION
    for(int i = 0; i<DECK_SIZE; i++){
        int j = rand() % DECK_SIZE;
        card_t temp = deck[i];
        deck[i] = deck[j];
        deck[j] = temp;
    }
}

//You dont need to use this if you dont want, but we did.
void init_game_state(game_state_t *game, int starting_stack, int random_seed){
    memset(game, 0, sizeof(game_state_t));
    init_deck(game->deck, random_seed);
    for (int i = 0; i < MAX_PLAYERS; i++) {
        game->player_stacks[i] = starting_stack;
    }
}

void reset_game_state(game_state_t *game) {
    shuffle_deck(game->deck);
    //Call this function between hands.
    //You should add your own code, I just wanted to make sure the deck got shuffled.

}

void server_join(game_state_t *game) {
    //This function was called to get the join packets from all players
    client_packet_t in_pkt;
    for (int i = 0; i < MAX_PLAYERS; i++){
        recv(game->sockets[i], &in_pkt, sizeof(in_pkt), 0);
        if (in_pkt.packet_type == JOIN) {
                game->player_status[i] = PLAYER_ACTIVE; // sets ALL player to 1
                printf("Player #%d joined\n", i);
        }
    }
}

int server_ready(game_state_t *game) {
    //This function updated the dealer and checked ready/leave status for all players
    client_packet_t in_pkt;
    int num_ready = 0;

    for(int i = 0; i < MAX_PLAYERS; i++) {
        if (game->player_status[i] == PLAYER_ACTIVE) {
            if (recv(game->sockets[i], &in_pkt, sizeof(in_pkt), 0) <= 0) {
                perror("recv failed in recv_packet");
                return -1;
            }

            switch (in_pkt.packet_type) {
                case READY:
                    game->player_status[i] = 1; // player active
                    num_ready++;
                    break;
                case LEAVE:
                    game->player_status[i] = 2; // left game
                    // disconnect
                    close(game->sockets[i]);
                    break;
                default:
                    break; // not meant to happen
            }
        }
        
    }
    print_game_state(game);
    return num_ready;
}

//This was our dealing function with some of the code removed (I left the dealing so we have the same logic)
// create packet info for each player and send packet info
// send NACK if player - indicates still their turn and resend packet info
// "raises" less than the call amount, 
// if you check after someone bets, 
// if you try to "raise" more than you have,
// if you send "READY" or "LEAVE" instead of check/call/raise/fold.
void server_deal(game_state_t *game) {
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (game->player_status[i] == PLAYER_ACTIVE) {
            game->player_hands[i][0] = game->deck[game->next_card++];
            game->player_hands[i][1] = game->deck[game->next_card++];
        }
    }
}

int server_bet(game_state_t *game) {
    //This was our function to determine if everyone has called or folded
    (void) game;
    return 0;
}

// Returns 1 if all bets are the same among active players
int check_betting_end(game_state_t *game) {
    (void)game;
    return 0; 
}

void server_community(game_state_t *game) {
    //This function checked the game state and dealt new community cards if needed;
    (void) game;
}

void server_end(game_state_t *game) {
    //This function sends the end packet
    info_packet_t info_pkt;
    
}

int evaluate_hand(game_state_t *game, player_id_t pid) {
    //We wrote a function to compare a "value" for each players hand (to make comparison easier)
    //Feel free to not do this.
    (void) game;
    (void) pid;
    return 0;
}

int find_winner(game_state_t *game) {
    //We wrote this function that looks at the game state and returns the player id for the best 5 card hand.
    (void) game;
    return -1;
}