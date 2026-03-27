#include "room.h"
#include <stdlib.h>
#include <string.h>


//Create
Room *room_create(int id, const char *name, int width, int height){

    //Allocates space for room
    Room *r = malloc(sizeof(Room));

    //Checks if room is null
    if (r == NULL){

        return NULL;
    
    }


    //Sets the room parameters (clamp dimensions to at least 1)
    r->id = id;
    r->width = (width < 1) ? 1 : width;
    r->height = (height < 1) ? 1 : height;


    //Checks if the name is null
    if (name != NULL){

        //allocates space for the name
        r->name = malloc(strlen(name) + 1);

        //checks if allocation is successful
        if (r->name == NULL){

            free(r);
            return NULL;

        }

        //copies the name into the room name
        strcpy(r->name, name);

    }else{

        r->name = NULL;

    }

    //Initializes other room parameters to NULL or 0
    r->floor_grid = NULL;
    r->portals = NULL;
    r->portal_count = 0;
    r->treasures = NULL;
    r->treasure_count = 0;

    // Ensure all newer fields are initialized as well
    r->neighbors = NULL;
    r->neighbor_count = 0;
    r->pushables = NULL;
    r->pushable_count = 0;
    r->switches = NULL;
    r->switch_count = 0;

    return r;

}


//Destroy
void room_destroy(Room *r){

    //Checks if the room is null
    if (r == NULL){

        return;//ends function

    }

    //Frees the name and floor grid of room
    free(r->name);
    free(r->floor_grid);


    //checks if there are portals
    if (r->portals != NULL){

        //runs through every portal
        for (int i = 0; i < r->portal_count; i++){

            free(r->portals[i].name);//frees all the portal names

        }

        //Frees the portals
        free(r->portals);

    }


    if (r->treasures != NULL){

        //runs through every treasure
        for (int i = 0; i < r->treasure_count; i++){

            //frees all the treasure names
            free(r->treasures[i].name);
        }

        //frees the treasures
        free(r->treasures);

    }

    //free pushables
    if (r->pushables != NULL){
        for (int i = 0; i < r->pushable_count; i++){
            free(r->pushables[i].name);
        }
        free(r->pushables);
    }

    //free switches
    free(r->switches);

    //free neighbors
    free(r->neighbors);


    free(r);//frees the room

}



int room_get_width(const Room *r){

    //NULL room
    if (r == NULL){

        return 0;

    }

    return r->width;

}




int room_get_height(const Room *r){

    //NULL room
    if (r == NULL){

        return 0;

    }

    return r->height;

}



Status room_set_floor_grid(Room *r, bool *floor_grid){

    //NULL room
    if (r == NULL){

        return INVALID_ARGUMENT;

    }

    //Free old grid
    free(r->floor_grid);

    //Set to new grid
    r->floor_grid = floor_grid;

    return OK;

}




Status room_set_portals(Room *r, Portal *portals, int portal_count){

    //Checks if room is NULL or if there is "portals" that are NULL and increase count
    if (r == NULL || (portal_count > 0 && portals == NULL)){

        return INVALID_ARGUMENT;
    }


    //Freeing the older portals
    for (int i = 0; i < r->portal_count; i++) {//free names
        free(r->portals[i].name);
    }

    //free portals
    free(r->portals);


    //Set to new portals and count
    r->portals = portals;
    r->portal_count = portal_count;

    return OK;

}




Status room_set_treasures(Room *r, Treasure *treasures, int treasure_count){

    //Checks if the room is NULL or if treasure count has treasure while treasures is NULL
    if (r == NULL || (treasure_count > 0 && treasures == NULL)){

        return INVALID_ARGUMENT;

    }

    //Free old treasures
    for (int i = 0; i < r->treasure_count; i++){//names
        
        free(r->treasures[i].name);
    
    }

    free(r->treasures);//treasures


    //Set new treasure and count
    r->treasures = treasures;
    r->treasure_count = treasure_count;


    return OK;

}



Status room_place_treasure(Room *r, const Treasure *treasure){

    //Checks if room or treasure is null
    if (r == NULL || treasure == NULL){

        return INVALID_ARGUMENT;

    }

    //Reallocates the treasure memory to a new array so that one more treasure can be added
    Treasure *new_array = realloc(r->treasures,sizeof(Treasure) * (r->treasure_count + 1));

    //checks failure to assign memory
    if (new_array == NULL){

        return NO_MEMORY;

    }


    //treasures now equals the new array with the added treasure
    r->treasures = new_array;


    //Copies the new treasure into the newly allocated space
    Treasure *destination = &r->treasures[r->treasure_count];//gets space allocated
    *destination = *treasure;//copies treasure into space


    //Checks if the treasure name is defined
    if (treasure->name != NULL){

        //Allocates memory for the new string name for the struct to own
        destination->name = strdup(treasure->name);

        //checks if allocation is successful
        if (destination->name == NULL){

            return NO_MEMORY;

        }
    }


    r->treasure_count++;
    return OK;

}



int room_get_treasure_at(const Room *r, int x, int y){

    //Checks if room is equal to NULL
    if (r == NULL) {

        return -1;

    }


    //Goes through all the treasures
    for (int i = 0; i < r->treasure_count; i++){

        // skip any already collected troves
        if (r->treasures[i].collected) {
            continue;
        }

        //Checks if the given x and y are equal to any treasure location
        if (r->treasures[i].x == x && r->treasures[i].y == y){

            return r->treasures[i].id;//returns the id

        }
    }

    return -1;//returns -1 if not found

}



int room_get_portal_destination(const Room *r, int x, int y){

    //Checks if room is NULL
    if (r == NULL) {

        return -1;
    
    }

    //Runs through every portal
    for (int i = 0; i < r->portal_count; i++){

        //Checks if any portal is equal to the given x and y
        if (r->portals[i].x == x && r->portals[i].y == y){

            return r->portals[i].target_room_id;//returns the portal destination room

        }
    }


    return -1;//returns -1 if not found

}



bool room_is_walkable(const Room *r, int x, int y){

    //Checks if room is NULL
    if (r == NULL) {

        return false;
    
    }


    //Checks the boundaries of the room
    if (x < 0 || y < 0 ||x >= r->width || y >= r->height){

        return false;

    }


    //Checks if the floor grid hasn't been made yet
    if (r->floor_grid == NULL){

        if (x == 0 || y == 0 || x == r->width - 1 || y == r->height - 1) {

            return false;

        }

        // tile is interior floor; still need to check pushables
        // fall through to pushable check below
    }

    bool walkable = false;
    if (r->floor_grid == NULL) {
        walkable = true;
    } else {
        //return true or false as floor_grid is a bool array
        //each row has width elements so y is the row and x is the elements in that row
        walkable = r->floor_grid[y * r->width + x];
    }

    if (!walkable) {
        return false;
    }

    // finally, a tile is not walkable if occupied by any pushable
    if (r->pushables) {
        for (int i = 0; i < r->pushable_count; ++i) {
            if (r->pushables[i].x == x && r->pushables[i].y == y) {
                return false;
            }
        }
    }

    return true;

}





//Helper function: validate room and tile coordinates
static bool room_tile_is_valid(const Room *r, int x, int y) {
    if (r == NULL) {
        return false;
    }
    if (x < 0 || x >= r->width || y < 0 || y >= r->height) {
        return false;
    }
    return true;
}

//Helper function: check for treasure at position
static RoomTileType room_check_treasure(const Room *r, int x, int y, int *out_id) {
    if (!r) {
        return ROOM_TILE_INVALID;
    }
    
    for (int i = 0; i < r->treasure_count; i++) {
        Treasure *my_t = &r->treasures[i];
        
        if (my_t->collected == false && my_t->x == x && my_t->y == y) {
            if (out_id != NULL) {
                *out_id = my_t->id;
            }
            return ROOM_TILE_TREASURE;
        }
    }
    
    return ROOM_TILE_INVALID;
}

//Helper function: check for portal at position
static RoomTileType room_check_portal(const Room *r, int x, int y, int *out_id) {
    if (!r) {
        return ROOM_TILE_INVALID;
    }
    
    for (int i = 0; i < r->portal_count; i++) {
        Portal *my_p = &r->portals[i];
        
        if (my_p->x == x && my_p->y == y) {
            if (out_id != NULL) {
                *out_id = my_p->target_room_id;
            }
            return ROOM_TILE_PORTAL;
        }
    }
    
    return ROOM_TILE_INVALID;
}

// Helper: resolve a switch by required ID/index and report pressed state 
static bool room_switch_is_pressed(const Room *r, int required_switch_id){
    if (r == NULL || r->switches == NULL || r->switch_count <= 0){
        return false;
    }

    const Switch *sw = NULL;

    if (required_switch_id >= 0 && required_switch_id < r->switch_count){
        sw = &r->switches[required_switch_id];
    } else {
        for (int i = 0; i < r->switch_count; ++i){
            if (r->switches[i].id == required_switch_id){
                sw = &r->switches[i];
                break;
            }
        }
    }

    if (sw == NULL){
        return false;
    }

    for (int i = 0; i < r->pushable_count; ++i){
        if (r->pushables[i].x == sw->x && r->pushables[i].y == sw->y){
            return true;
        }
    }

    return false;
}

static char room_base_tile_char(const Room *r, const Charset *charset, int x, int y){
    if (r->floor_grid == NULL){
        bool is_border = (x == 0 || y == 0 || x == r->width - 1 || y == r->height - 1);
        if (is_border){
            return charset->wall;
        }
        return charset->floor;
    }

    int idx = y * r->width + x;
    if (r->floor_grid[idx]){
        return charset->floor;
    }
    return charset->wall;
}

static void room_render_base_tiles(const Room *r, const Charset *charset, char *buffer){
    for (int y = 0; y < r->height; y++){
        for (int x = 0; x < r->width; x++){
            int idx = y * r->width + x;
            buffer[idx] = room_base_tile_char(r, charset, x, y);
        }
    }
}

static void room_render_treasures(const Room *r, const Charset *charset, char *buffer){
    for (int i = 0; i < r->treasure_count; i++){
        Treasure *my_t = &r->treasures[i];
        if (my_t->collected == false){
            int idx = my_t->y * r->width + my_t->x;
            buffer[idx] = (char)charset->treasure;
        }
    }
}

static void room_render_portals(const Room *r, const Charset *charset, char *buffer){
    for (int i = 0; i < r->portal_count; i++){
        Portal *my_p = &r->portals[i];
        int idx = my_p->y * r->width + my_p->x;
        buffer[idx] = (char)charset->portal;
    }
}

static void room_render_switches(const Room *r, const Charset *charset, char *buffer){
    for (int i = 0; i < r->switch_count; i++){
        Switch *sw = &r->switches[i];
        int idx = sw->y * r->width + sw->x;
        bool pressed = room_switch_is_pressed(r, sw->id);
        if (pressed){
            buffer[idx] = charset->switch_on;
        } else {
            buffer[idx] = charset->switch_off;
        }
    }
}

static void room_render_pushables(const Room *r, const Charset *charset, char *buffer){
    for (int i = 0; i < r->pushable_count; i++){
        Pushable *my_push = &r->pushables[i];
        int idx = my_push->y * r->width + my_push->x;
        buffer[idx] = (char)charset->pushable;
    }
}


RoomTileType room_classify_tile(const Room *r,int x,int y,int *out_id){

    if (!room_tile_is_valid(r, x, y)) {
        return ROOM_TILE_INVALID;
    }

    //Check for treasure at this position
    RoomTileType treasure_type = room_check_treasure(r, x, y, out_id);
    if (treasure_type == ROOM_TILE_TREASURE) {
        return ROOM_TILE_TREASURE;
    }

    //Check for portal at this position
    RoomTileType portal_type = room_check_portal(r, x, y, out_id);
    if (portal_type == ROOM_TILE_PORTAL) {
        return ROOM_TILE_PORTAL;
    }

    // Pushables take precedence over floor/wall; an occupied tile is not walkable
    if (r->pushables) {
        for (int i = 0; i < r->pushable_count; ++i) {
            if (r->pushables[i].x == x && r->pushables[i].y == y) {
                if (out_id != NULL) {
                    *out_id = i;
                }
                return ROOM_TILE_PUSHABLE;
            }
        }
    }

    //checks if the tile is walkable
    if (room_is_walkable(r, x, y) == true){

        return ROOM_TILE_FLOOR;//returns a floor tile

    }


    return ROOM_TILE_WALL;//Returns that tile is a wall

}





Status room_render(const Room *r,const Charset *charset,char *buffer,int buffer_width,int buffer_height){
    
    //checks if any of the arguments are null
    if (r == NULL || charset == NULL || buffer == NULL){

        return INVALID_ARGUMENT;

    }

    //checks if the buffer dimensions are equal to the room dimensions
    if (buffer_width != r->width || buffer_height != r->height){

        return INVALID_ARGUMENT;

    }


    room_render_base_tiles(r, charset, buffer);
    room_render_treasures(r, charset, buffer);
    room_render_portals(r, charset, buffer);
    room_render_switches(r, charset, buffer);
    room_render_pushables(r, charset, buffer);


    return OK;

}




Status room_get_start_position(const Room *r,int *x_out,int *y_out){

    //checks if any arguments are null
    if (r == NULL || x_out == NULL || y_out == NULL){

        return INVALID_ARGUMENT;

    }


    //Checks if there is any portals
    if (r->portal_count > 0){

        //If there is portals choose the first one
        *x_out = r->portals[0].x;
        *y_out = r->portals[0].y;

        return OK;

    }


    //Runs through all tiles
    for (int my_y = 0; my_y < r->height; my_y++){


        for (int my_x = 0; my_x < r->width; my_x++){


            //Checks if a tile is walkable
            if (room_is_walkable(r, my_x, my_y) == true){

                //Finds a walkable tile and sets start to the first tile found
                *x_out = my_x;
                *y_out = my_y;


                return OK;

            }
        }
    }


    return ROOM_NOT_FOUND;

}


//A2 additions: additional room utilities

int room_get_id(const Room *r){

    //Checks if room exists
    if (r == NULL){ 

        return -1;

    }

    return r->id;

}

Status room_pick_up_treasure(Room *r, int treasure_id, Treasure **treasure_out){

    //Checks if room or output pointer is null, also checks if treasure_id is negative
    if(r == NULL || treasure_out == NULL){

        return INVALID_ARGUMENT;

    }

    if(treasure_id < 0){

        return INVALID_ARGUMENT;

    }


    //Runs through all the treasures
    for(int i = 0; i < r->treasure_count; ++i){
        
        //Sets the treasure pointer to the current treasure
        Treasure *t = &r->treasures[i];
        
        //Checks if the current treasure has the same id as the given treasure_id
        if(t->id == treasure_id){
            
            //Checks if the treasure is collected
            if(t->collected){
                
                return INVALID_ARGUMENT;
            
            }
            
            //If not collected set to collected and set output parameter to the treasure
            t->collected = true;
            *treasure_out = t;
            return OK;

        }
    }

    //Otherwise room is not found
    return ROOM_NOT_FOUND;

}

void destroy_treasure(Treasure *t){

    //Checks if treasure exists
    if (t == NULL){ 
        
        return;

    }

    //Frees the name and the treasure itself
    free(t->name);
    free(t);

}


bool room_has_pushable_at(const Room *r,
                          int x,
                          int y,
                          int *pushable_idx_out){

    //Checks if room exists
    if (r == NULL){

        return false;

    }

    //Runs through all the pushables
    for(int i = 0; i < r->pushable_count; ++i){

        //Checks if the pushable is at the given x and y
        if (r->pushables[i].x == x && r->pushables[i].y == y){

            if(pushable_idx_out != NULL){

                //Put the index into the output parameter
                *pushable_idx_out = i;
                
            }

            //Return success
            return true;

        }
    }

    //Return failure
    return false;

}

Status room_try_push(Room *r,
                     int pushable_idx,
                     Direction dir) {

    //Checks if room exists, if pushable index is valid, and if direction is valid
    if(r == NULL || pushable_idx < 0 || pushable_idx >= r->pushable_count){

        return INVALID_ARGUMENT;

    }

    //Checks if direction is valid
    if(dir < DIR_NORTH || dir > DIR_WEST){

        return INVALID_ARGUMENT;

    }

    //Gets the pushable to be pushed
    Pushable *p = &r->pushables[pushable_idx];
    int dx = 0;
    int dy = 0;

    //Find which direction the pushable is going
    switch (dir){
        case DIR_NORTH: dy = -1; break;
        case DIR_SOUTH: dy = 1; break;
        case DIR_EAST: dx = 1; break;
        case DIR_WEST: dx = -1; break;
        default: break;
    }

    //Compute new coordinates
    int newx = p->x + dx;
    int newy = p->y + dy;

    // check destination tile - must be walkable floor, can't be treasure/portal/pushable/wall
    int dummy_id = 0;
    RoomTileType tile_type = room_classify_tile(r, newx, newy, &dummy_id);
    if(tile_type != ROOM_TILE_FLOOR){

        return ROOM_IMPASSABLE;

    }

    //Set new coordinates
    p->x = newx;
    p->y = newy;

    return OK;
}

