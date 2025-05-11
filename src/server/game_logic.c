#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <stdbool.h>

#include "poker_client.h"
#include "client_action_handler.h"
#include "game_logic.h"

typedef struct { 
    // 9 = STRAIGHT FLUSH
    // 8 = FOUR OF A KIND ...
    int category; 
    int ranks[5]; // for tie breaking 
} hand_value_t;

int advance_to_next_player(game_state_t *game);
void sort5_desc(int arr[5]);
hand_value_t eval_hand(game_state_t *game, player_id_t pid);
hand_value_t evaluate_5_card_val(card_t cards[5]);
int compare(const hand_value_t *hand1, const hand_value_t *hand2);



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
    // at beginning of game dealer should be 0; initialize to -1 so when ready is called updates properly
    // edge case: beginning of game
    game->dealer_player = -1;
}

void reset_game_state(game_state_t *game) {
    shuffle_deck(game->deck);

    //Call this function between hands.
    //You should add your own code, I just wanted to make sure the deck got shuffled.
    // zero out community cards, ...
    for (int i = 0; i < 5; i++) {
        game->community_cards[i] = 0;
    }
    game->next_card = 0;
    for (int i = 0; i < 5; i++) {
        game->player_stacks[i] = 0;
    }
    for (int i = 0; i < 5; i++) {
        game->current_bets[i] = 0;
    }
    game->highest_bet = 0;
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (game->player_status[i] != PLAYER_LEFT) {
            game->player_status[i] = PLAYER_ACTIVE;
        }
    }
    game->pot_size = 0;

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

// if 1 player or none HALT returns -1
int server_ready(game_state_t *game) {
    //This function updated the dealer and checked ready/leave status for all players
    client_packet_t ready_pkt;
    game->round_stage = ROUND_INIT;
    int num_ready = 0;
    for(int i = 0; i < MAX_PLAYERS; i++) {
        if (game->player_status[i] == PLAYER_ACTIVE) {
            if (recv(game->sockets[i], &ready_pkt, sizeof(ready_pkt), 0) <= 0) {
                printf("recv failed in recv_packet");
                return -1;
            }

            switch (ready_pkt.packet_type) {
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

    // HALT
    if (num_ready < 2) {
        // if one person left send halt msg and close their port
        server_packet_t halt_pkt;
        halt_pkt.packet_type = HALT;
        for (int i = 0; i < MAX_PLAYERS; i++) {
            if (game->player_status[i] == PLAYER_ACTIVE) {
                send(game->sockets[i], &halt_pkt, sizeof(server_packet_t), 0);
                close(game->sockets[i]);
            }
        }
        return -1;
    }

    // update dealer
    // start of game, player 0 goes first or lowest number ava.
    if (game->dealer_player == -1) {
        for (int i = 0; i < MAX_PLAYERS; i++) {
            if (game->player_status[i] == PLAYER_ACTIVE) {
                game->dealer_player = i;
                break;
            }
        }
        // curr player is updated to after
        int curr_player = (game->dealer_player + 1) % MAX_PLAYERS;
        while (game->player_status[curr_player] != PLAYER_ACTIVE) {
            curr_player = (curr_player + 1) % MAX_PLAYERS;
        }
        game->current_player = curr_player;
    } else {
    // rounds thereafter
        int old_dealer = game->dealer_player;
        int new_dealer = (old_dealer + 1) % MAX_PLAYERS;
        // check if next player is ava if not continue, mod to loop back to 0
        while (game->player_status[new_dealer] != PLAYER_ACTIVE) {
            new_dealer = (new_dealer + 1) % MAX_PLAYERS;
            // if at any point we loop back to old dealer there is a problem! should not happen
            if (new_dealer == old_dealer)
                printf("new_dealer = old dealer");
        }
        game->dealer_player = new_dealer;
        // update current player
        int curr_player = (game->dealer_player + 1) % MAX_PLAYERS;
        while (game->player_status[curr_player] != PLAYER_ACTIVE) {
            curr_player = (curr_player + 1) % MAX_PLAYERS;
        }
        game->current_player = curr_player;
    }
    print_game_state(game);
    game->num_players = num_ready;
    return num_ready;
}

//This was our dealing function with some of the code removed (I left the dealing so we have the same logic)
// create packet info for each player and send packet info

void server_deal(game_state_t *game) {
    for (int i = 0; i < 5; i++) 
        game->community_cards[i] = NOCARD; // (NOCARD ~ -1)

    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (game->player_status[i] == PLAYER_ACTIVE) {
            game->player_hands[i][0] = game->deck[game->next_card++];
            game->player_hands[i][1] = game->deck[game->next_card++];
            // send info packet to each player who is PLAYER_ACTIVE 
            server_packet_t out;
            build_info_packet(game, i, &out);
            if (send(game->sockets[i], &out, sizeof(server_packet_t), 0) <= 0){
                printf("send packet fail in dealing stage");
            }
        }
    }
    
}

// send NACK if player - indicates still their turn and resend packet info
// "raises" less than the call amount, 
// if you check after someone bets, 
// if you try to "raise" more than you have,
// if you send "READY" or "LEAVE" instead of check/call/raise/fold.
// if at any point everyone folds but one person, that person automatically wins the hand
int server_bet(game_state_t *game) {
    //This was our function to determine if everyone has called or folded
    bool gone_to_everyone = false;
    int first_player = game->current_player;
    for(;;) {
        int current_player = game->current_player; 
        client_packet_t in;
        server_packet_t out;

        if (recv(game->sockets[current_player], &in, sizeof(in), 0) <= 0 ) {
            printf("error receiving packet in server_bet");
        }

        handle_client_action(game, current_player, &in, &out);

        switch (out.packet_type) {
            case ACK:
                // send an ACK msg to player
                send(game->sockets[current_player], &out, sizeof(server_packet_t), 0);
                
                // advance to next player unless betting has ended
                int next_player = advance_to_next_player(game);
                // update current player
                game->current_player = next_player;

                // if the next person to go is the first player, we looped once! 
                if (next_player == first_player) 
                    gone_to_everyone = true;

                if (gone_to_everyone && check_betting_end(game)) {
                    return 1; // betting round over!!
                }

                // resend info to every person at table (all in, active player, folded)
                for (int i = 0; i < MAX_PLAYERS; i++) {
                    if (game->player_status[i] == PLAYER_ACTIVE || game->player_status[i] == PLAYER_FOLDED) {
                        server_packet_t new_info;
                        build_info_packet(game, i, &new_info);
                        send(game->sockets[i], &new_info, sizeof(server_packet_t), 0);
                    }
                }
                break;

            case NACK:
                // send NACK
                send(game->sockets[current_player], &out, sizeof(server_packet_t), 0);
                // resend info packet to same player
                server_packet_t same_info;
                build_info_packet(game, current_player, &same_info);
                send(game->sockets[current_player], &same_info, sizeof(server_packet_t), 0);
            default:
                printf("error in server_bet for out switch case");
        }
    }
}

// helper function to advance to next player, returns next_player pid
int advance_to_next_player(game_state_t *game) {
    int current_player = game->current_player;
    int next_player = (current_player + 1) % MAX_PLAYERS;
    while (game->player_status[next_player] != PLAYER_ACTIVE) {
        next_player = (next_player + 1) % MAX_PLAYERS;
    }
    return next_player;
}

// Returns 1 if all bets are the same among active players
int check_betting_end(game_state_t *game) {
    int all_are_same = 1;
    int bet = -1;
    // find first bet
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (game->player_status[i] == PLAYER_ACTIVE) {
            bet = game->current_bets[i]; 
            break;
        } 
    }
    // compare w/ rest of bets
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (game->player_status[i] == PLAYER_ACTIVE) {
            if (bet != game->current_bets[i]) {
                return 0;
            }
        } 
    }
    return 1; // all bets are same
}

void server_community(game_state_t *game) {
    //This function checked the game state and dealt new community cards if needed;
    switch (game->round_stage) {
        case ROUND_FLOP:
            for (int i = 0; i < 3; i++) 
                game->community_cards[i] = game->deck[game->next_card++];
            break;
        case ROUND_TURN:
            game->community_cards[3] = game->deck[game->next_card++];
            break;
        case ROUND_RIVER:
            game->community_cards[4] = game->deck[game->next_card++];
            break;
        default:
            break;
    }
   
    // send packet to each active player / folded player
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (game->player_status[i] == PLAYER_ACTIVE || game->player_status[i] == PLAYER_FOLDED) {
            server_packet_t out;
            build_info_packet(game, i, &out);
            if (send(game->sockets[i], &out, sizeof(server_packet_t), 0) <= 0){
                printf("send packet fail in community flop stage");
            }
        }
    }
}

void server_end(game_state_t *game) {
    //This function sends the end packet
    server_packet_t end;
    build_end_packet(game, find_winner(game), &end);
    
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (game->player_status[i] == PLAYER_ACTIVE)
            send(game->sockets[i], &end, sizeof(server_packet_t), 0);
    }
}

/*
lowest to highest
    HAND_HIGH_CARD = 1,
    HAND_ONE_PAIR = 2,
    HAND_TWO_PAIR = 3,
    HAND_THREE_OF_A_KIND = 4,
    HAND_STRAIGHT = 5,
    HAND_FLUSH = 6,
    HAND_FULL_HOUSE = 7,
    HAND_FOUR_OF_A_KIND = 8,
    HAND_STRAIGHT_FLUSH = 9
*/
/*
#define RANK(c)  ((c) >> SUITE_BITS)   // 0..12
#define SUITE(c) ((c) & ((1 << SUITE_BITS) - 1))  // 0..3
2,3,4,5,6,7,8,9,10,J,Q,K,A
Example: RANK( ACE | SPADE ) = 12
         SUITE( ACE | SPADE ) = 3
*/
/*
When you enter “showdown”, the way to determine who has the best hand is by comparing the best 5 card hand for each player. This means each player can use 0, 1, or both of their hole cards alongside 3-5 of the public “community” cards. The ordering of poker hands from best to worst is as follows:

Straight-Flush: 5 cards in consecutive order of the same suit (ex: 3h, 4h, 5h, 6h, 7h)
4-of-a-kind: 4 cards that all have the same numerical value (ex: Kh, Ks, Kc, Kd, 7s)
Full House: a pair and three of a kind (ex: Kh, Ks, 4d, 4s, 4c)
Flush: 5 cards with the same suit (ex: 3h, 8h, 9h, Kh, Ah)
Straight: 5 cards in consecutive order (ex: 9h, 10s, Jd, Qc, Kc)
Three-of-a-kind: 3 cards with the same numerical value (ex: 2h, 2c, 2d, 6c, 8h)
Two Pair: 2 groups of 2 cards with the same numerical value (ex: 4h, 4c, 6d, 8d, 8s)
One Pair: 2 cards with the same numerical value (ex: 2h, 2c, 4d, 6c, 8h)
High Card: None of the above hands.

If two players have the same hand then there is a way to break ties.
For straights and straight flushes, the player with the highest number in the straight wins
	2-6 loses to 3-7 (A-5 is lowest straight; 10-A is highest; K, A, 2, 3, 4 is not a straight).
For a Pair, 3 or 4-of-a-kind the player with the higher number being matched wins.
	3,3,3,5,6 loses to 2,3,5,5,5
For Full House and Two pair you use the above rule but compare:
3-of-a-kind first, then higher pair, then lower pair
For Flush and High card you compare the highest unique number
	2h, 3h, 6h, 7h, Ah beats 3h, 6h, 7h, Qh, Kh but not 2h, 3h, 4h, Kh, Ah
    */

// compare two hands if hand1 > hand2 return pos number if hand1 < hand2 return neg number if = return 0
int compare(const hand_value_t *hand1, const hand_value_t *hand2) {
    // compare category
    if (hand1->category > hand2->category) {
        return 1;
    } else if (hand1->category < hand2->category) {
        return -1;
    }

    // if equal compare ranks (same category: straight, flush etc...)
    for (int i = 0; i < 5; i++) {
        if (hand1->ranks[i] > hand2->ranks[i]) {
            return 1;
        } else if (hand1->ranks[i] < hand2->ranks[i]) {
            return -1;
        }
    }
    // equal
    return 0;
}

hand_value_t evaluate_5_card_val(card_t cards[5]) {
    hand_value_t hv;
    memset(&hv, 0, sizeof(hv));
    
    // 1 extract rank/suit in arrays
    int rank[5], suit[5];
    for(int i=0; i<5; i++){
        rank[i] = RANK(cards[i]);
        suit[i] = SUITE(cards[i]);
    }
    
    // sort rank array descending: bubble sort
    for (int i=0; i<5; i++) {
        for (int j=i+1; j<5; j++) {
            if (rank[j] > rank[i]) {
                int rtemp = rank[i];
                rank[i] = rank[j];
                rank[j] = rtemp;
                
                int stemp = suit[i];
                suit[i] = suit[j];
                suit[j] = stemp;
            }
        }
    }
    
    // check for flush (all same suit)
    bool is_flush = true;
    for (int i=1; i<5; i++){
        if(suit[i] != suit[0]){
            is_flush = false;
            break;
        }
    }
    
    // Check for straight:
    // AKQJ10 is highest straight
    // "Ace-low" case: A2345 => ranks would be [12, 3, 2, 1, 0] 
    bool is_straight = true;
    int top_straight_rank = rank[0]; 
    for (int i=1; i<5; i++){
        if(rank[i] != rank[i-1] - 1) {
            is_straight = false;
            break;
        }
    }
    // Special check for A2345
    // sorted descending is [12, 3, 2, 1, 0]
    // If that pattern is found, the straight top rank we use is 3 (the '5')
    if (!is_straight) {
        // check A2345 specifically
        // rank[0] = 12 (Ace), rank[1] = 3, rank[2] = 2, rank[3] = 1, rank[4] = 0
        bool ace_low = (rank[0] == 12 && rank[1] == 3 && rank[2] == 2 && rank[3] == 1 && rank[4] == 0);
        if(ace_low) {
            is_straight = true;
            top_straight_rank = 3; // (5 for A2345)
        }
    }

    // count duplicates using frequency of each rank
    int freq_count[13];
    memset(freq_count, 0, sizeof(freq_count));
    for(int i=0; i<5; i++){
        freq_count[rank[i]]++;
    }
    // how many of each (4-of-a-kind, 3-of-a-kind, pairs)
    // which rank is which
    int four_rank = -1, three_rank = -1;
    int pair_rank[2]; // might be up to 2 pairs
    pair_rank[0] = pair_rank[1] = -1;
    int pair_index = 0;
    
    for (int r=0; r<13; r++){
        if (freq_count[r] == 4) four_rank = r;
        if (freq_count[r] == 3) three_rank = r;
        if (freq_count[r] == 2) {
            if (pair_index < 2) {
                pair_rank[pair_index++] = r;
            }
        }
    }
    
    // categorize & fill hv.ranks for tie break
    if (is_flush && is_straight) {
        // Straight flush
        hv.category = 9; // STRAIGHT_FLUSH
        hv.ranks[0] = top_straight_rank;
    }

    else if (four_rank != -1) {
        // Four of a kind
        hv.category = 8;
        hv.ranks[0] = four_rank; 
        // other card
        for (int i=0; i<5; i++){
            if (rank[i] != four_rank){
                hv.ranks[1] = rank[i];
                break;
            }
        }
    }
    else if (three_rank != -1 && pair_rank[0] != -1) {
        // Full house
        hv.category = 7;
        hv.ranks[0] = three_rank; // priority
        hv.ranks[1] = pair_rank[0];
    }
    else if(is_flush) {
        // Flush
        hv.category = 6;
        // all 5 ranks descending
        for(int i=0; i<5; i++){
            hv.ranks[i] = rank[i];
        }
    }
    else if(is_straight) {
        // straight
        hv.category = 5;
        hv.ranks[0] = top_straight_rank;
    }
    else if(three_rank != -1) {
        // Three of a kind
        hv.category = 4;
        hv.ranks[0] = three_rank;
        // two other cards in descending order
        int idx = 1;
        for(int i=0; i<5; i++){
            if(rank[i] != three_rank){
                hv.ranks[idx++] = rank[i];
            }
        }
    }
    else if(pair_rank[0] != -1 && pair_rank[1] != -1) {
        // Two Pair
        hv.category = 3;
        // higher pair rank first
        int highp = (pair_rank[0] > pair_rank[1]) ? pair_rank[0] : pair_rank[1];
        int lowp  = (pair_rank[0] > pair_rank[1]) ? pair_rank[1] : pair_rank[0];
        hv.ranks[0] = highp;
        hv.ranks[1] = lowp;
        // other card
        for(int i=0; i<5; i++){
            if(rank[i] != highp && rank[i] != lowp){
                hv.ranks[2] = rank[i];
                break;
            }
        }
    }
    else if(pair_rank[0] != -1) {
        // One Pair
        hv.category = 2;
        hv.ranks[0] = pair_rank[0];
        // then the 3 other cards
        int idx = 1;
        for(int i=0; i<5; i++){
            if(rank[i] != pair_rank[0]){
                hv.ranks[idx++] = rank[i];
            }
        }
    }
    else {
        // high card
        hv.category = 1;
        for(int i=0; i<5; i++){
            hv.ranks[i] = rank[i];
        }
    }

    return hv;
}

// evaluate best 5 card hand out of 7
hand_value_t eval_hand(game_state_t *game, player_id_t pid) {
    //We wrote a function to compare a "value" for each players hand (to make comparison easier)
    //Feel free to not do this.
    card_t all_cards[7];
    all_cards[0] = game->player_hands[pid][0];
    all_cards[1] = game->player_hands[pid][1];
    for (int i = 0; i < 5; i++) {
        all_cards[i + 2] = game->community_cards[i];
    }

    // defaulted category is 0 lowest ! good for compare
    hand_value_t best_hand;
    memset(&best_hand, 0, sizeof(best_hand));

    // check every 5 combination possibility, order doesn't matter 7c5 ways to check
    // pass in subsets and get the best of 5 hands returned
    // 0,1,2,3,4
    // 0,1,2,3,5
    // 0,1,2,3,6
    // 0,1,2,4,5
    // 0,1,2,4,6
    // 0,1,3,4,5
    for (int i = 0; i < 7; i++) {
        for (int j = i+1; j < 7; j++) {
            for (int k = j+1; k < 7; k++) {
                for (int l = k+1; l < 7; l++) {
                    for (int m = l+1; m < 7; m++) {
                        card_t subset[5];
                        subset[0] = all_cards[i];
                        subset[1] = all_cards[j];
                        subset[2] = all_cards[k];
                        subset[3] = all_cards[l];
                        subset[4] = all_cards[m];

                        hand_value_t hand_val = evaluate_5_card_val(subset);
                        if (compare(&hand_val, &best_hand) > 0)
                            best_hand = hand_val;
                    }
                }
            }
        }
    }
    
    return best_hand;
}

// returns player id with best 5 card hands
int find_winner(game_state_t *game) {
    //We wrote this function that looks at the game state and returns the player id for the best 5 card hand.
    player_id_t winner = -1;
    hand_value_t winning_hand;
    memset(&winning_hand, 0, sizeof(winning_hand));

    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (game->player_status[i] == PLAYER_ACTIVE) {
            hand_value_t players_best_hand = eval_hand(game, i); 

            if (winner == -1) {
                winner = i;
                winning_hand = players_best_hand;
            } else {
                if (compare(&players_best_hand, &winning_hand) > 0) {
                    winning_hand = players_best_hand;
                    winner = i;
                }
            }
        }
       
    }

    return winner;
}