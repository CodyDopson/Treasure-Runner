#include "world_loader.h"
#include "room.h"
#include "datagen.h"
#include "graph.h"

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

//Helper function to convert datagen status codes to loader status codes
Status dg_status_helper(int dg_status){
    
    if (dg_status == -1){
        return WL_ERR_CONFIG;
    }
    
    if (dg_status == -2){
        return NO_MEMORY;
    }
    
    return WL_ERR_DATAGEN;
}


//Compare based on id, allows ordering by id
static int room_compare_fn(const void *a, const void *b){

    const Room *ra = a;
    const Room *rb = b;

    if (ra->id < rb->id){ 
        
        return -1;

    }

    if (ra->id > rb->id) {
        
        return 1;
    
    }

    return 0;

}


//Destroy
static void room_destroy_wrapper(void *payload){

    Room *r = payload;
    room_destroy(r);
    
}


/* Type definitions for room management */
typedef struct { int id; Room *r; } RoomEntry;
typedef struct { int from; int to; } PendingEdge;


/* Helper to copy portals from datagen room to student room and collect edges */
static Status copy_portals_to_room(const DG_Room *dg, Room *r,PendingEdge **edges, int *edges_count, int *edges_capacity){
    
    //Checks if there are portals
    if (dg->portal_count <= 0 || !dg->portals){
        return OK;
    }
    
    //Allocates space for portals
    Portal *p = malloc(sizeof(Portal) * dg->portal_count);
    if (!p){
        return NO_MEMORY;
    }
    
    //Copies all the portal data and gets edges
    for (int i = 0; i < dg->portal_count; ++i){

        //Portal data
        p[i].id = dg->portals[i].id;
        p[i].name = NULL; /* datagen portals have no name */
        p[i].x = dg->portals[i].x;
        p[i].y = dg->portals[i].y;
        p[i].target_room_id = dg->portals[i].neighbor_id;
        p[i].gated = (dg->portals[i].required_switch_id >= 0);
        p[i].required_switch_id = dg->portals[i].required_switch_id;


        if (dg->portals[i].neighbor_id >= 0){

            //Checks if array size needs increasing
            if (*edges_count + 1 > *edges_capacity){

                //Doubles the capacity if needed
                int newcap = *edges_capacity ? *edges_capacity * 2 : 8;

                //increases space
                PendingEdge *tmp = realloc(*edges, sizeof(PendingEdge) * newcap);
                if (!tmp){
                    free(p);
                    return NO_MEMORY;
                }

                *edges = tmp;
                *edges_capacity = newcap;

            }
            //Adds edge to the list of edges to connect after all rooms are inserted
            (*edges)[(*edges_count)++] = (PendingEdge){ .from = dg->id, .to = dg->portals[i].neighbor_id };
        }
    }
    
    //sets the portals of the room to the copied portals
    room_set_portals(r, p, dg->portal_count);
    return OK;
}



/* Helper to copy pushables from datagen room to student room */
static Status copy_pushables_to_room(const DG_Room *dg, Room *r){
    if (dg->pushable_count <= 0 || !dg->pushables){
        return OK;
    }

    Pushable *p = malloc(sizeof(Pushable) * dg->pushable_count);
    if (!p){
        return NO_MEMORY;
    }

    for (int i = 0; i < dg->pushable_count; ++i){
        p[i].id = dg->pushables[i].id;
        p[i].name = dg->pushables[i].name ? strdup(dg->pushables[i].name) : NULL;
        p[i].initial_x = dg->pushables[i].x;
        p[i].initial_y = dg->pushables[i].y;
        p[i].x = dg->pushables[i].x;
        p[i].y = dg->pushables[i].y;
    }

    r->pushables = p;
    r->pushable_count = dg->pushable_count;
    return OK;
}


/* Helper to copy switches from datagen room to student room */
static Status copy_switches_to_room(const DG_Room *dg, Room *r){
    if (dg->switch_count <= 0 || !dg->switches){
        return OK;
    }

    Switch *s = malloc(sizeof(Switch) * dg->switch_count);
    if (!s){
        return NO_MEMORY;
    }

    for (int i = 0; i < dg->switch_count; ++i){
        s[i].id = dg->switches[i].id;
        s[i].x = dg->switches[i].x;
        s[i].y = dg->switches[i].y;
        s[i].portal_id = dg->switches[i].portal_id;
    }

    r->switches = s;
    r->switch_count = dg->switch_count;
    return OK;
}


/* Helper to copy treasures from datagen room to student room */
static Status copy_treasures_to_room(const DG_Room *dg, Room *r){
    
    //Checks if there are treasures
    if (dg->treasure_count <= 0 || !dg->treasures){
        return OK;
    }
    
    //Allocates space for treasures
    Treasure *t = malloc(sizeof(Treasure) * dg->treasure_count);
    if (!t){
        return NO_MEMORY;
    }
    
    for (int i = 0; i < dg->treasure_count; ++i){

        //copies the treasure data
        t[i].id = dg->treasures[i].global_id;
        t[i].name = dg->treasures[i].name ? strdup(dg->treasures[i].name) : NULL;
        t[i].starting_room_id = dg->id;
        t[i].initial_x = dg->treasures[i].x;
        t[i].initial_y = dg->treasures[i].y;
        t[i].x = dg->treasures[i].x;
        t[i].y = dg->treasures[i].y;
        t[i].collected = false;
    }
    
    //sets the treasures of the room to the copied treasures
    room_set_treasures(r, t, dg->treasure_count);
    return OK;
    
}


/* Helper to insert rooms into the graph */
static Status insert_rooms_into_graph(Graph *g, RoomEntry *rooms, int rooms_count){
    
    //Runs through all the rooms and inserts them into the graph
    for (int i = 0; i < rooms_count; ++i){
        GraphStatus s = graph_insert(g, rooms[i].r);

        //checks success
        if (s != GRAPH_STATUS_OK){
            return (s == GRAPH_STATUS_NO_MEMORY) ? NO_MEMORY : INTERNAL_ERROR;
        }
    }
    
    return OK;
}

/* Helper to connect edges between rooms */
static void connect_edges(Graph *g, PendingEdge *edges, int edges_count, RoomEntry *rooms, int rooms_count){
    
    //runs through all the edges
    for (int i = 0; i < edges_count; ++i){

        //Finds the rooms corresponding to the edge
        Room *from = NULL;
        Room *to = NULL;

        //Runs through all rooms to find the from and to rooms for the edge
        for (int j = 0; j < rooms_count; ++j){

            if (rooms[j].id == edges[i].from){ 
                
                from = rooms[j].r;
            }

            if (rooms[j].id == edges[i].to){
                
                to = rooms[j].r;
                
            }

            if (from && to){
                
                break;

            }

        }

        if (from && to){

            graph_connect(g, from, to);

        }
    }
}



static Status validate_loader_args(const char *config_file,
                                   Graph **graph_out,
                                   Room **first_room_out,
                                   const int *num_rooms_out,
                                   Charset *charset_out)
{
    //Checks if any thing is null
    if (!config_file || !graph_out || !first_room_out ||!num_rooms_out || !charset_out){

        return INVALID_ARGUMENT;

    }
    return OK;
}



static void load_charset(Charset *out)
{
    const DG_Charset *dg_cs = dg_get_charset();

    //Loads charset from datagen if present 
    if (dg_cs != NULL){
        out->wall = dg_cs->wall;
        out->floor = dg_cs->floor;
        out->player = dg_cs->player;
        out->treasure = dg_cs->treasure;
        out->portal = dg_cs->portal;
        out->pushable = dg_cs->pushable;
        out->switch_off = dg_cs->switch_off;
        out->switch_on = dg_cs->switch_on;
    } else {
        out->wall = '#';
        out->floor = '.';
        out->player = '@';
        out->treasure = 'T';
        out->portal = 'P';
        out->pushable = '?';
        out->switch_off = 's';
        out->switch_on = 'S';
    }
}



/* Cleanup helper: destroys current room, all previously loaded rooms, edges, and graph */
static void cleanup_load_state(
    Room *r,
    RoomEntry *rooms,
    int rooms_count,
    PendingEdge *edges,
    Graph *g
){
    room_destroy(r);
    stop_datagen();
    for (int i = 0; i < rooms_count; ++i){
        room_destroy(rooms[i].r);
    }
    free(rooms);
    free(edges);
    graph_destroy(g);
}


static Status handle_room_portals(
    Graph *g,
    const DG_Room *dg,
    Room *r,
    RoomEntry **rooms,
    const int *rooms_count,
    PendingEdge **edges,
    int *edges_count,
    int *edges_capacity
){
    //Copies portals and gets edges
    Status portal_status = copy_portals_to_room(
        dg, r,
        edges, edges_count, edges_capacity
    );

    //Checks success
    if (portal_status != OK){
        cleanup_load_state(r, *rooms, *rooms_count, *edges, g);
        return portal_status;
    }

    return OK;
}




static Status handle_room_treasures(
    Graph *g,
    const DG_Room *dg,
    Room *r,
    RoomEntry **rooms,
    const int *rooms_count,
    PendingEdge **edges
){

    Status treasure_status = copy_treasures_to_room(dg, r);

    //Destroys everything if copying fails
    if (treasure_status != OK){
        cleanup_load_state(r, *rooms, *rooms_count, *edges, g);
        return treasure_status;
    }

    return OK;
}




static Status ensure_room_capacity(
    Graph *g,
    Room *r,
    RoomEntry **rooms,
    const int *rooms_count,
    int *rooms_capacity,
    PendingEdge **edges
){

    //Checks if there is capacity for another room
    if (*rooms_count + 1 <= *rooms_capacity){
        return OK;
    }

    //Doubles capacity if needed
    int newcap = *rooms_capacity ? *rooms_capacity * 2 : 8;
    RoomEntry *tmp = realloc(*rooms, sizeof(RoomEntry) * newcap);

    //Destroys everything if realloc fails
    if (!tmp){
        cleanup_load_state(r, *rooms, *rooms_count, *edges, g);
        return NO_MEMORY;
    }

    *rooms = tmp;
    *rooms_capacity = newcap;
    return OK;
}




/* Helper to copy the floor grid from datagen into the room */
static Status copy_floor_grid_to_room(const DG_Room *dg, Room *r){
    if (!dg->floor_grid){
        return OK;
    }

    bool *grid = malloc(sizeof(bool) * dg->width * dg->height);
    if (!grid){
        return NO_MEMORY;
    }

    memcpy(grid, dg->floor_grid, sizeof(bool) * dg->width * dg->height);
    room_set_floor_grid(r, grid);
    return OK;
}


static Status load_rooms_from_datagen(
    Graph *g,
    RoomEntry **rooms,
    int *rooms_count,
    int *rooms_capacity,
    PendingEdge **edges,
    int *edges_count,
    int *edges_capacity
){

    //Runs through all rooms from datagen
    while (has_more_rooms()){
        DG_Room dg = get_next_room();

        Room *r = room_create(dg.id, NULL, dg.width, dg.height);

        //Checks if creation was successful
        if (!r){
            cleanup_load_state(NULL, *rooms, *rooms_count, *edges, g);
            return NO_MEMORY;
        }

        /* copy floor grid if present */
        Status grid_status = copy_floor_grid_to_room(&dg, r);
        if (grid_status != OK){
            cleanup_load_state(r, *rooms, *rooms_count, *edges, g);
            return grid_status;
        }

        Status portal_status = handle_room_portals(
            g, &dg, r, rooms, rooms_count,
            edges, edges_count, edges_capacity
        );
        if (portal_status != OK){
            return portal_status;
        }

        Status treasure_status = handle_room_treasures(
            g, &dg, r, rooms, rooms_count, edges
        );
        if (treasure_status != OK){
            return treasure_status;
        }

        // Copy pushables from datagen to room
        Status pushable_status = copy_pushables_to_room(&dg, r);
        if (pushable_status != OK){
            cleanup_load_state(r, *rooms, *rooms_count, *edges, g);
            return pushable_status;
        }

        // Copy switches from datagen to room
        Status switch_status = copy_switches_to_room(&dg, r);
        if (switch_status != OK){
            cleanup_load_state(r, *rooms, *rooms_count, *edges, g);
            return switch_status;
        }

        Status cap_status = ensure_room_capacity(
            g, r, rooms, rooms_count, rooms_capacity, edges
        );
        if (cap_status != OK){
            return cap_status;
        }

        //Adds room to the list of rooms for post-processing and graph insertion
        (*rooms)[(*rooms_count)++] = (RoomEntry){ .id = dg.id, .r = r };
    }

    return OK;
}



Status loader_load_world(const char *config_file,Graph **graph_out,Room **first_room_out,int *num_rooms_out,Charset *charset_out){


    //Checks if any paramater is equal to null
    Status s = validate_loader_args(config_file, graph_out,first_room_out, num_rooms_out,charset_out);

    if (s != OK){
        
        return s;

    }

    //Starts datagen
    int dg_status = start_datagen(config_file);


    //Checks if an error occurred
    if (dg_status != 0){


        return dg_status_helper(dg_status);
        
    }



    load_charset(charset_out);


    Graph *g = NULL;
    GraphStatus gs = graph_create(room_compare_fn, room_destroy_wrapper, &g);
    if (gs != GRAPH_STATUS_OK){
        stop_datagen();
        return (gs == GRAPH_STATUS_NO_MEMORY) ? NO_MEMORY : INTERNAL_ERROR;
    }

    //Dynamic arrays to keep track for post-processing edges 
    RoomEntry *rooms = NULL;
    int rooms_count = 0;
    int rooms_capacity = 0;

    PendingEdge *edges = NULL;
    int edges_count = 0;
    int edges_capacity = 0;


    Status load_status = load_rooms_from_datagen(
    g,
    &rooms,
    &rooms_count,
    &rooms_capacity,
    &edges,
    &edges_count,
    &edges_capacity
    );


    if (load_status != OK){

    return load_status;

    }

    /* Copied all rooms; datagen can be stopped */
    stop_datagen();

    /* insert rooms into graph */
    Status insert_status = insert_rooms_into_graph(g, rooms, rooms_count);

    if (insert_status != OK){

        for (int j = 0; j < rooms_count; ++j){
            
            room_destroy(rooms[j].r);
        
        }

        free(rooms);
        free(edges);
        graph_destroy(g);
        return insert_status;
    }

    /* connect edges */
    connect_edges(g, edges, edges_count, rooms, rooms_count);

    *graph_out = g;
    *first_room_out = (rooms_count > 0) ? rooms[0].r : NULL;
    *num_rooms_out = rooms_count;


    free(rooms);
    free(edges);

    return OK;

}
