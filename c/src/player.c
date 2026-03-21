#include "player.h"
#include <stdlib.h>
#include <string.h>


Status player_create(int initial_room_id, int initial_x, int initial_y, Player **player_out){


    Player *player = NULL;

    //check if player_out is null
    if (player_out == NULL){

        return INVALID_ARGUMENT;

    }

    // Allocate memory for the player 
    player = malloc(sizeof(Player));

    //check if fails
    if (player == NULL){

        return NO_MEMORY;

    }

    // Initialize player fields 
    player->room_id = initial_room_id;
    player->x = initial_x;
    player->y = initial_y;

    //Start with no collected treasures
    player->collected_treasures = NULL;
    player->collected_count = 0;

    //set player_out to the created player
    *player_out = player;

    //returns success
    return OK;


}


void player_destroy(Player *p){
    if (!p) {
        return;
    }

    /* free collected treasure array but not the treasures themselves */
    free(p->collected_treasures);
    p->collected_treasures = NULL;
    p->collected_count = 0;

    free(p);
    p = NULL;
}


int player_get_room(const Player *p){

    if (p == NULL) {
        return -1;
    }

    return p->room_id;

}



Status player_get_position(const Player *p, int *x_out, int *y_out){

    if (p == NULL || x_out == NULL || y_out == NULL){

        return INVALID_ARGUMENT;

    }

    *x_out = p->x;
    *y_out = p->y;

    return OK;

}



Status player_set_position(Player *p, int x, int y){

    if (p == NULL){

        return INVALID_ARGUMENT;
    
    }


    p->x = x;
    p->y = y;

    return OK;

}


Status player_move_to_room(Player *p, int new_room_id){

    //Check if player is null
    if (p == NULL){

        return INVALID_ARGUMENT;

    }

    p->room_id = new_room_id;

    return OK;

}



Status player_reset_to_start(Player *p, int starting_room_id, int start_x, int start_y){
    
    //Check if player is null
    if (p == NULL){

        return INVALID_ARGUMENT;

    }

    //Reset player position to starting position
    p->room_id = starting_room_id;
    p->x = start_x;
    p->y = start_y;

    //Also clear collected treasures
    for(int i = 0; i < p->collected_count; ++i){

        if(p->collected_treasures[i]){

            p->collected_treasures[i]->collected = false;

        }
    }

    //Free the collected treasures array but not the treasures themselves
    free(p->collected_treasures);
    p->collected_treasures = NULL;
    p->collected_count = 0;

    return OK;

}



Status player_try_collect(Player *p, Treasure *treasure){

    //Check if player or treasure is null
    if(p == NULL || treasure == NULL){

        return NULL_POINTER;

    }

    //Check if player has already collected this treasure
    if(player_has_collected_treasure(p, treasure->id) == true){

        return INVALID_ARGUMENT;

    }

    //allocate space for one more Treasure* pointer
    Treasure **new_arr = realloc(p->collected_treasures,
                                 sizeof(Treasure *) * (p->collected_count + 1));
                
    if(new_arr == NULL){

        return NO_MEMORY;

    }


    //Add the new treasure to the end of the array and update count
    p->collected_treasures = new_arr;
    p->collected_treasures[p->collected_count] = treasure;
    p->collected_count++;
    treasure->collected = true;


    return OK;

}


bool player_has_collected_treasure(const Player *p, int treasure_id){

    //Check if player or treasure_id is invalid
    if(p == NULL || treasure_id < 0){

        return false;

    }

    //Iterate through collected treasures to see if any match the given treasure_id
    for(int i = 0; i < p->collected_count; ++i){

        //Check if the current collected treasure matches the given treasure_id
        if(p->collected_treasures[i] && p->collected_treasures[i]->id == treasure_id){

            return true;

        }

    }

    return false;

}

int player_get_collected_count(const Player *p){

    //Check if player is null
    if(p == NULL){

        return 0;

    }

    return p->collected_count;

}

const Treasure * const *
player_get_collected_treasures(const Player *p, int *count_out){

    //Check if player or count_out is null
    if(p == NULL || count_out == NULL){

        return NULL;

    }

    //Sets the ouptut parameter to the count of collected treasures
    *count_out = p->collected_count;

    //Returns the array of collected treasures
    return (const Treasure * const *)p->collected_treasures;

}
