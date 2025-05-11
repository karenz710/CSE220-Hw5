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
    // send NACK if player - indicates still their turn and resend packet info
    // "raises" less than the call amount, 
    // if you check after someone bets, 
    // if you try to "raise" more than you have,
    // if you send "READY" or "LEAVE" instead of check/call/raise/fold.

    int highest_bet = game->highest_bet; // must call check to this amount 
    // or raise higher than the call amount
    int player_current_bet = game->current_bets[pid];
    int player_to_call = highest_bet - player_current_bet;

    switch (in->packet_type) {
        case CHECK:
            // checking is when the highest bet is 0 
            if (player_to_call == 0) {
                out->packet_type = ACK;
                return 1;
                // nothing is changed
            }  
        default:
            break;
        /*
         case CALL:
            // call is when the highest bet is > 0
            if (player_to_call > 0) {
                // You can only ALL IN through a raise or a call. 
                // NOT tested if you can split the pot correctly, you can just award the full pot to the winner.


            } else {
                // meant to check
            
            }
        case RAISE:
            int raise_amount = in->params[0];
            if (raise_amount < player_to_call) {
                out->packet_type = NACK;

            } 

        case FOLD:
            game->player_status[pid] = PLAYER_FOLDED;
            default:
            break;
            // check if 2nd to last person who folded
            int tot_fold = 0;
            for (int i = 0; i < MAX_PLAYERS; i++) {
                if (game->player_status[i] == PLAYER_FOLDED) {
                    tot_fold++;
                }
            }
            if (tot_fold == game->num_players - 1) {
                // skip to end packet and winner is remaining player
                server_packet_t end;
    build_end_packet(game, find_winner(game), &end);
    
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (game->player_status[i] == PLAYER_ACTIVE)
            send(game->sockets[i], &end, sizeof(server_packet_t), 0);
    }
            }
            */
            
            
            // sent READY or LEAVE reply NACK and resent packet info
    }
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
    out->packet_type = END;
    for (int i = 0; i < MAX_PLAYERS; i++) {
        out->end.player_cards[i][0] = game->player_hands[i][0];
        out->end.player_cards[i][1] = game->player_hands[i][1];
    }
    //community cards
    for (int i = 0; i < 5; i++) {
        out->end.community_cards[i] = game->community_cards[i];
    }
    // player stacks
    for (int i = 0; i < MAX_PLAYERS; i++) {
        out->end.player_stacks[i] = game->player_stacks[i];
    }
    // pot size
    out->end.pot_size = game->pot_size;

    // dealer (old dealer from finished hand)
    out->end.dealer = game->dealer_player;

    // winner //ignore chopped pots
    out->end.winner = winner;

    // player status 
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (game->player_status[i] == PLAYER_ACTIVE || game->player_status[i] == PLAYER_ALLIN) {
            out->end.player_status[i] = 1; // active
        } else if (game->player_status[i] == PLAYER_FOLDED) {
            out->end.player_status[i] = 0; //folded
        } else {
            out->end.player_status[i] = 2; //left
        }
    }

}
