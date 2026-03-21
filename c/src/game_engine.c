#include "game_engine.h"
#include "world_loader.h"
#include "room.h"
#include "player.h"
#include "graph.h"

#include <stdlib.h>
#include <string.h>


//Helper function: Get room by ID from graph
static Room *get_room_by_id(Graph *g, int room_id){


    //Checks if the graph is null
    if (g == NULL){

        return NULL;

    }
    
    //Creates a temp room struct that copies only the id and sets everything to 0/null
    Room temp = { .id = room_id };


    //Retrieve the room payload from the graph
    return (Room *)graph_get_payload(g, &temp);

}





//Create and Destroy
Status game_engine_create(const char *config_file_path, GameEngine **engine_out){


    //Checks if the parameters are null
    if (config_file_path == NULL || engine_out == NULL){

        return INVALID_ARGUMENT;

    }

    //Allocates space for the game engine
    GameEngine *eng = malloc(sizeof(GameEngine));

    //Checks succes of allocation
    if (eng == NULL){

        return NO_MEMORY;

    }


    //Creates initial specifications
    Room *first_room = NULL;
    int num_rooms = 0;

    // Load world using loader
    Status load_status = loader_load_world(config_file_path, &eng->graph, &first_room, &num_rooms, &eng->charset);


    //Checks if the load was successful
    if (load_status != OK){

        //If not successful free engine and return error
        free(eng);

        //Returns the error of the failed load
        return load_status;

    }



    //Sets the room count of the engine to the number of rooms loaded
    eng->room_count = num_rooms;


    //Variables for getting starting position from first room
    int start_x = 0;
    int start_y = 0;


    //Gets starting position from first room
    Status pos_status = room_get_start_position(first_room, &start_x, &start_y);


    //Checks if getting position was successful
    if (pos_status != OK){

        //Switch back to (0, 0) if we can't find a valid start position
        start_x = 0;
        start_y = 0;

    }


    //Sets the initial player position and room id
    eng->initial_room_id = first_room->id;
    eng->initial_player_x = start_x;
    eng->initial_player_y = start_y;


    // Create player at starting position
    Status player_status = player_create(eng->initial_room_id,eng->initial_player_x,eng->initial_player_y,&eng->player);

    //Checks if player was created successful
    if (player_status != OK){

        //Destroys the graph and frees engine
        graph_destroy(eng->graph);
        free(eng);

        //Returns the error of player creation
        return player_status;

    }

    //Sets engine out to the created engine
    *engine_out = eng;

    //Returns success
    return OK;

}





void game_engine_destroy(GameEngine *eng){


    //Checks if eng is null
    if (eng == NULL){

        return;//exits

    }


    //Destroys player and graph
    player_destroy(eng->player);
    graph_destroy(eng->graph);

    //Frees the engine
    free(eng);

}





const Player *game_engine_get_player(const GameEngine *eng){

    //Checks if the engine is NULL
    if (eng == NULL){

        return NULL;

    }


    return eng->player;//Returns the player in the engine

}


/* Helper: check if a gated portal's switch is pressed */
static bool is_switch_pressed(const Room *room, int switch_id){
    if (switch_id < 0 || switch_id >= room->switch_count){
        return false;
    }
    Switch *sw = &room->switches[switch_id];
    for (int j = 0; j < room->pushable_count; ++j){
        if (room->pushables[j].x == sw->x && room->pushables[j].y == sw->y){
            return true;
        }
    }
    return false;
}


/* Helper: check if a portal at (x,y) is traversable */
static bool portal_is_traversable(const Room *room, int x, int y, int target_room_id){
    if (target_room_id < 0){
        return false;
    }
    for (int i = 0; i < room->portal_count; ++i){
        Portal *p = &room->portals[i];
        if (p->x == x && p->y == y && p->gated){
            return is_switch_pressed(room, p->required_switch_id);
        }
    }
    return true;
}


/* Helper: handle treasure tile interaction */
static Status handle_treasure_tile(GameEngine *eng, Room *room, int tile_id, int next_x, int next_y){
    if (player_has_collected_treasure(eng->player, tile_id)){
        return player_set_position(eng->player, next_x, next_y);
    }

    Treasure *treasure = NULL;
    Status pickup_status = room_pick_up_treasure(room, tile_id, &treasure);
    if (pickup_status != OK){
        return pickup_status;
    }

    Status collect_status = player_try_collect(eng->player, treasure);
    if (collect_status != OK){
        return collect_status;
    }
    return OK;
}


/* Helper: handle pushable tile interaction */
static Status handle_pushable_tile(GameEngine *eng, Room *room, int tile_id, Direction dir, int next_x, int next_y){
    Status push_status = room_try_push(room, tile_id, dir);
    if (push_status != OK){
        return push_status;
    }
    return player_set_position(eng->player, next_x, next_y);
}


/* Helper: handle portal tile interaction */
static Status handle_portal_tile(GameEngine *eng, Room *current_room, int tile_id, int next_x, int next_y){
    if (!portal_is_traversable(current_room, next_x, next_y, tile_id)){
        return player_set_position(eng->player, next_x, next_y);
    }

    Room *new_room = get_room_by_id(eng->graph, tile_id);
    if (new_room == NULL){
        return GE_NO_SUCH_ROOM;
    }

    Status move_status = player_move_to_room(eng->player, tile_id);
    if (move_status != OK){
        return move_status;
    }

    int entry_x = 0;
    int entry_y = 0;
    Status pos_status = room_get_start_position(new_room, &entry_x, &entry_y);
    if (pos_status != OK){
        entry_x = 0;
        entry_y = 0;
    }
    return player_set_position(eng->player, entry_x, entry_y);
}


/* Helper: compute the next position for a given direction */
static Status compute_next_position(int current_x, int current_y, Direction dir, int *next_x, int *next_y){
    *next_x = current_x;
    *next_y = current_y;

    switch (dir){
        case DIR_NORTH: (*next_y)--; break;
        case DIR_SOUTH: (*next_y)++; break;
        case DIR_EAST:  (*next_x)++; break;
        case DIR_WEST:  (*next_x)--; break;
        default: return INVALID_ARGUMENT;
    }
    return OK;
}




Status game_engine_move_player(GameEngine *eng, Direction dir){

    if (eng == NULL || eng->player == NULL || eng->graph == NULL){
        return INVALID_ARGUMENT;
    }

    int next_x = 0;
    int next_y = 0;
    Status dir_status = compute_next_position(eng->player->x, eng->player->y, dir, &next_x, &next_y);
    if (dir_status != OK){
        return dir_status;
    }

    Room *current_room = get_room_by_id(eng->graph, eng->player->room_id);
    if (current_room == NULL){
        return GE_NO_SUCH_ROOM;
    }

    int tile_id = -1;
    RoomTileType tile_type = room_classify_tile(current_room, next_x, next_y, &tile_id);

    switch (tile_type){
        case ROOM_TILE_TREASURE:
            return handle_treasure_tile(eng, current_room, tile_id, next_x, next_y);
        case ROOM_TILE_PUSHABLE:
            return handle_pushable_tile(eng, current_room, tile_id, dir, next_x, next_y);
        case ROOM_TILE_PORTAL:
            return handle_portal_tile(eng, current_room, tile_id, next_x, next_y);
        case ROOM_TILE_FLOOR:
            return player_set_position(eng->player, next_x, next_y);
        default:
            return ROOM_IMPASSABLE;
    }

}





Status game_engine_get_room_count(const GameEngine *eng, int *count_out){
    
    
    //Checks if the engine is null
    if (eng == NULL){

        return INVALID_ARGUMENT;

    }


    //Checks if count out is null
    if (count_out == NULL){

        return NULL_POINTER;

    }

    //Gets the room count from the engine
    *count_out = eng->room_count;


    return OK;//returns the status

}




Status game_engine_get_room_dimensions(const GameEngine *eng,int *width_out,int *height_out){

    //Checks if the engine is null
    if (eng == NULL){

        return INVALID_ARGUMENT;

    }


    //Checks if output parameters are null
    if (width_out == NULL || height_out == NULL){

        return NULL_POINTER;

    }



    //Checks if player is null
    if (eng->player == NULL){

        return INTERNAL_ERROR;

    }


    //Get current room
    Room *room = get_room_by_id(eng->graph, eng->player->room_id);


    //Checks if room is null
    if (room == NULL){

        return GE_NO_SUCH_ROOM;

    }


    //Sets output parameters to room dimensions
    *width_out = room_get_width(room);
    *height_out = room_get_height(room);


    return OK;

}





Status game_engine_reset(GameEngine *eng){


    //Checks if engine is null
    if (eng == NULL){

        return INVALID_ARGUMENT;

    }

    //Checks if player is null (internal state error)
    if (eng->player == NULL){

        return INTERNAL_ERROR;

    }


    //Resets player to initial position
    Status reset_status = player_reset_to_start(eng->player,eng->initial_room_id,eng->initial_player_x,eng->initial_player_y);

    if (reset_status != OK){
        return reset_status;
    }

    //Reset pushables to their initial positions across all rooms
    const void * const *payloads = NULL;
    int payload_count = 0;
    GraphStatus gs = graph_get_all_payloads(eng->graph, &payloads, &payload_count);
    if (gs != GRAPH_STATUS_OK){
        return INTERNAL_ERROR;
    }

    for (int i = 0; i < payload_count; i++){
        Room *room = (Room *)payloads[i];
        //Reset pushables
        for (int j = 0; j < room->pushable_count; j++){
            room->pushables[j].x = room->pushables[j].initial_x;
            room->pushables[j].y = room->pushables[j].initial_y;
        }
    }

    return OK;

}





Status game_engine_render_current_room(const GameEngine *eng, char **str_out){


    //Checks if eng or str_out is null
    if (eng == NULL || str_out == NULL){

        return INVALID_ARGUMENT;

    }

    //Checks if player is null
    if (eng->player == NULL){

        return INTERNAL_ERROR;

    }


    //Get current room
    Room *room = get_room_by_id(eng->graph, eng->player->room_id);

    //Checks if room is null
    if (room == NULL){
        return GE_NO_SUCH_ROOM;
    }

    // Render current room with player overlay
    int width = room_get_width(room);
    int height = room_get_height(room);

    int buffer_size = width * height + height + 1;
    char *buffer = malloc((size_t)buffer_size);
    if (buffer == NULL) return NO_MEMORY;

    char *flat_buffer = malloc((size_t)width * (size_t)height);
    if (flat_buffer == NULL) { free(buffer); return NO_MEMORY; }

    Status render_status = room_render(room, &eng->charset, flat_buffer, width, height);
    if (render_status != OK) {
        free(buffer);
        free(flat_buffer);
        return render_status;
    }

    // overlay player
    if (eng->player != NULL && eng->player->room_id == room->id) {
        int pidx = eng->player->y * width + eng->player->x;
        flat_buffer[pidx] = eng->charset.player;
    }

    // format with newlines
    int pos = 0;
    //Runs through all heights
    for (int yy = 0; yy < height; yy++) {

        //Runs through all widths
        for (int xx = 0; xx < width; xx++) {

            //Copies the character from the flat buffer to the final buffer
            buffer[pos++] = flat_buffer[yy * width + xx];//finds spot in flat buffer by y as row and x as element

        }

        //Format with newlines at the end of each row
        buffer[pos++] = '\n';

    }

    //Null terminates the string
    buffer[pos] = '\0';

    //Frees the flat buffer and sets the output string to the final buffer
    free(flat_buffer);
    *str_out = buffer;

    return OK;

}




Status game_engine_render_room(const GameEngine *eng, int room_id, char **str_out){


    //Checks if eng is null
    if (eng == NULL){

        return INVALID_ARGUMENT;

    }

    //Checks if str_out is null
    if (str_out == NULL){

        return NULL_POINTER;

    }


    //Get room by ID
    Room *room = get_room_by_id(eng->graph, room_id);


    //Checks if room is null
    if (room == NULL){

        return GE_NO_SUCH_ROOM;

    }


    // Get room dimensions
    int width = room_get_width(room);
    int height = room_get_height(room);



    // Allocate buffer for room (width*height + height for newlines)
    int buffer_size = width * height + height + 1;



    //Allocates space for a buffer
    char *buffer = malloc(buffer_size);

    
    //Checks if allocation was successful
    if (buffer == NULL) {

        return NO_MEMORY;

    }



    // Render room into flat buffer without newlines
    char *flat_buffer = malloc((size_t)width * (size_t)height);


    //Checks if allocation was successful
    if (flat_buffer == NULL){

        //Frees the first buffer
        free(buffer);
        return NO_MEMORY;

    }


    // Render room into flat buffer without newlines
    Status render_status = room_render(room, &eng->charset, flat_buffer, width, height);


    //Checks if rendering was successful
    if (render_status != OK){


        //Frees allocated buffers
        free(buffer);
        free(flat_buffer);


        //Returns the rendering status
        return render_status;

    }


    // Format with newlines
    int pos = 0;

    //runs through the entire height and width 
    //to copy over the flat buffer into the final buffer with newlines
    for (int my_y = 0; my_y < height; my_y++) {

        for (int my_x = 0; my_x < width; my_x++) {

            //copies the character from the flat buffer 
            // to the final buffer, y is row and x is element
            buffer[pos++] = flat_buffer[my_y * width + my_x];//decrements after code is run

        }

        //newline character at the end of each row
        buffer[pos++] = '\n';
    }

    //null terminates the string
    buffer[pos] = '\0';


    //frees the flat buffer and sets the output parameter to the final buffer
    free(flat_buffer);
    *str_out = buffer;


    //Return success status
    return OK;

}




Status game_engine_get_room_ids(const GameEngine *eng,int **ids_out,int *count_out){


    //Checks if the engine is null                                
    if (eng == NULL){

        return INVALID_ARGUMENT;

    }

    //Checks if the output parameters are null
    if (ids_out == NULL || count_out == NULL){

        return NULL_POINTER;

    }


    // Get all payloads from graph
    const void * const *payloads = NULL;

    int payload_count = 0;

    //gets all the payloads from the graph
    GraphStatus gs = graph_get_all_payloads(eng->graph, &payloads, &payload_count);


    //Checks if retrieval had an error
    if (gs != GRAPH_STATUS_OK){

        return INTERNAL_ERROR;
    
    }


    // Allocate array for room IDs
    int *ids = malloc(sizeof(int) * payload_count);
   
    
    //Checks if allocation was successful
    if (ids == NULL){

        return NO_MEMORY;

    }



    // Extract IDs from room payloads
    for (int i = 0; i < payload_count; i++){

        //Gets the room id from the payload and inserts into the array
        const Room *r = (const Room *)payloads[i];
        ids[i] = r->id;

    }


    //inserts the array and count into the output parameters
    *ids_out = ids;
    *count_out = payload_count;


    return OK;//returns success

}

void game_engine_free_string(void *ptr) {

    //Frees the string allocated by the game engine rendering functions
    free(ptr);
    
}
