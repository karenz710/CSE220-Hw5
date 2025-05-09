#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "client_action_handler.h"
#include "game_logic.h"

/**
 * @brief Processes packet from client and generates a server response packet.
 * 
 * If the action is valid, a SERVER_INFO packet will be generated containing the updated game state.
 * If the action is invalid or out of turn, a SERVER_NACK packet is returned with an optional error message.
 * 
 * @param pid The ID of the client/player who sent the packet.
 * @param in Pointer to the client_packet_t received from the client.
 * @param out Pointer to a server_packet_t that will be filled with the response.
 * @return 0 if successful processing, -1 on NACK or error.
 */
int handle_client_action(game_state_t *game, player_id_t pid, const client_packet_t *in, server_packet_t *out) {
    //Optional function, see documentation above. Strongly reccomended.
    (void)game;
    (void)pid;
    (void)in;
    (void)out;
    return -1;
}

void build_info_packet(game_state_t *game, player_id_t pid, server_packet_t *out) {
    //Put state info from "game" (for player pid) into packet "out"

    out->packet_type = INFO;
    out->info.player_cards[0] = game->player_hands[pid][0];
    out->info.player_cards[1] = game->player_hands[pid][1];
    
    memcpy(
        out->info.community_cards, // dest
        game->community_cards, // src
        sizeof(out->info.community_cards)
    );

    for (int i = 0; i < MAX_PLAYERS; i++) 
        out->info.player_stacks[i] = game->player_stacks[i]; 

    out->info.pot_size = game->pot_size;
    out->info.dealer = game->dealer_player;
    out->info.player_turn = game->current_player;
    out->info.bet_size = 0; 
    for (int i = 0; i < MAX_PLAYERS; i++) 
        out->info.player_bets[i] = game->current_bets[i]; 

    for (int i = 0; i < MAX_PLAYERS; i++) 
        out->info.player_status[i] = game->player_status[i]; 

}

void build_end_packet(game_state_t *game, player_id_t winner, server_packet_t *out) {
    //Put state info from "game" (and calculate winner) into packet "out"
    (void) game;
    (void) winner;
    (void) out;
}
