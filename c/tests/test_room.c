#include <check.h>
#include <stdlib.h>
#include <string.h>

#include "room.h"


static Room *room = NULL;
static Treasure treasure;
static Portal portal;
static Charset charset;


//Setup and teardown
static void setup_room(void){


    //Creates a starter room
    room = room_create(1, "Test Room", 5, 5);
    
    
    ck_assert_ptr_nonnull(room);
    
    // default charset for render tests
    charset.wall = '#';
    charset.floor = '.';
    charset.player = '@';
    charset.treasure = 'T';
    charset.portal = 'P';
    charset.pushable = '?';

    //Treasure setup
    treasure.id = 1;
    treasure.name = strdup("Treasure");
    treasure.starting_room_id = 1;

    treasure.initial_x = 2;
    treasure.initial_y = 2;

    treasure.x = 2;
    treasure.y = 2;

    treasure.collected = false;

    //Portal Setup
    portal.id = 1;
    portal.name = strdup("Portal");

    portal.x = 3;
    portal.y = 3;

    portal.target_room_id = 2;

}




static void teardown_room(void){

    room_destroy(room);
    room = NULL;


}


START_TEST(test_room_create_success){

    Room *myR = room_create(10, "CreateTest", 4, 3);
    ck_assert_ptr_nonnull(myR);

    ck_assert_int_eq(myR->id, 10);
    ck_assert_int_eq(myR->width, 4);
    ck_assert_int_eq(myR->height, 3);
    ck_assert_ptr_nonnull(myR->name);
    ck_assert_str_eq(myR->name, "CreateTest");

    ck_assert_ptr_null(myR->floor_grid);
    ck_assert_ptr_null(myR->portals);
    ck_assert_int_eq(myR->portal_count, 0);
    ck_assert_ptr_null(myR->treasures);
    ck_assert_int_eq(myR->treasure_count, 0);

    room_destroy(myR);

}
END_TEST


START_TEST(test_room_create_null_name){

    Room *myR = room_create(11, NULL, 2, 2);
    ck_assert_ptr_nonnull(myR);
    ck_assert_ptr_null(myR->name);
    room_destroy(myR);

}
END_TEST


START_TEST(test_room_destroy_null){

    room_destroy(NULL);

}
END_TEST


START_TEST(test_room_destroy_with_contents){

    //Creates a room with the id 2 with 3x3 dimensions
    Room *myR = room_create(2, "HasContents", 3, 3);

    ck_assert_ptr_nonnull(myR);



    //Mallocs space for a floor grid and sets all to true(for floor tile)
    bool *grid = malloc(sizeof(bool) * 9);

    for (int i = 0; i < 9; ++i){
        
        grid[i] = true;
    
    }

    //Checks if floor grid is successfully set in the room
    ck_assert_int_eq(room_set_floor_grid(myR, grid), OK);


    //Mallocs space for a portal
    Portal *myP = malloc(sizeof(Portal) * 1);

    //Sets portal id and name
    myP[0].id = 5;
    myP[0].name = strdup("p1");

    //Sets portal position and target room id
    myP[0].x = 0; 
    myP[0].y = 0; 
    myP[0].target_room_id = 99;


    //Check success of portal
    ck_assert_int_eq(room_set_portals(myR, myP, 1), OK);


    //Allocates space for treasure
    Treasure *myT = malloc(sizeof(Treasure) * 1);


    //Initializes id, name and room id
    myT[0].id = 6;
    myT[0].name = strdup("gold");
    myT[0].starting_room_id = 2;

    //Initializes position and collected status
    myT[0].initial_x = myT[0].x = 1;
    myT[0].initial_y = myT[0].y = 1;
    myT[0].collected = false;


    //Checks success of treasure
    ck_assert_int_eq(room_set_treasures(myR, myT, 1), OK);


    room_destroy(myR);

}
END_TEST



START_TEST(test_room_get_width_success){

    ck_assert_int_eq(room_get_width(room), 5);

}
END_TEST


START_TEST(test_room_get_width_null){

    ck_assert_int_eq(room_get_width(NULL), 0);

}
END_TEST


START_TEST(test_room_get_height_success){

    ck_assert_int_eq(room_get_height(room), 5);

}
END_TEST


START_TEST(test_room_get_height_null){

    ck_assert_int_eq(room_get_height(NULL), 0);

}
END_TEST




START_TEST(test_room_is_walkable_true){

    ck_assert(room_is_walkable(room, 2, 2) == true);

}
END_TEST


START_TEST(test_room_is_walkable_wall){

    ck_assert(room_is_walkable(room, 0, 0) == false);

}
END_TEST


START_TEST(test_room_is_walkable_out_of_bounds){

    ck_assert(room_is_walkable(room, -1, 0) == false);

}
END_TEST


START_TEST(test_room_is_walkable_null){

    ck_assert(room_is_walkable(NULL, 2, 2) == false);

}
END_TEST




START_TEST(test_room_place_treasure_success){
    
    ck_assert_int_eq(room_place_treasure(room, &treasure), OK);

}
END_TEST


START_TEST(test_room_place_treasure_null_room){

    ck_assert_int_eq(room_place_treasure(NULL, &treasure), INVALID_ARGUMENT);

}
END_TEST


START_TEST(test_room_get_treasure_at_found){

    room_place_treasure(room, &treasure);
    ck_assert_int_eq(room_get_treasure_at(room, 2, 2), 1);

}
END_TEST


START_TEST(test_room_get_treasure_at_not_found){

    ck_assert_int_eq(room_get_treasure_at(room, 1, 1), -1);

}
END_TEST


START_TEST(test_room_get_treasure_at_null){

    ck_assert_int_eq(room_get_treasure_at(NULL, 2, 2), -1);

}
END_TEST




START_TEST(test_room_get_portal_destination_found){

    Portal *portals = malloc(sizeof(Portal));

    portals[0] = portal;

    room_set_portals(room, portals, 1);

    ck_assert_int_eq(room_get_portal_destination(room, 3, 3), 2);

}
END_TEST


START_TEST(test_room_get_portal_destination_not_found){

    ck_assert_int_eq(room_get_portal_destination(room, 1, 1), -1);

}
END_TEST


START_TEST(test_room_get_portal_destination_null){

    ck_assert_int_eq(room_get_portal_destination(NULL, 3, 3), -1);

}
END_TEST




START_TEST(test_room_classify_tile_treasure){

    int out_id = -1;
    room_place_treasure(room, &treasure);


    ck_assert_int_eq(room_classify_tile(room, 2, 2, &out_id),ROOM_TILE_TREASURE);

    ck_assert_int_eq(out_id, 1);

}
END_TEST


START_TEST(test_room_classify_tile_portal){

    int out_id = -1;

    Portal *portals = malloc(sizeof(Portal));

    portals[0] = portal;

    room_set_portals(room, portals, 1);


    ck_assert_int_eq(room_classify_tile(room, 3, 3, &out_id),ROOM_TILE_PORTAL);

    ck_assert_int_eq(out_id, 2);

}
END_TEST


START_TEST(test_room_classify_tile_wall){

    ck_assert_int_eq(room_classify_tile(room, 0, 0, NULL),ROOM_TILE_WALL);
    
}
END_TEST


START_TEST(test_room_classify_tile_null){

    ck_assert_int_eq(room_classify_tile(NULL, 1, 1, NULL),ROOM_TILE_INVALID);

}
END_TEST




START_TEST(test_room_render_success){

    char buffer[25];

    ck_assert_int_eq(room_render(room, &charset, buffer, 5, 5), OK);

}
END_TEST


START_TEST(test_room_render_null_room){

    char buffer[25];

    ck_assert_int_eq(room_render(NULL, &charset, buffer, 5, 5), INVALID_ARGUMENT);

}
END_TEST


START_TEST(test_room_get_start_position_portal){

    int myX = -1, myY = -1;

    Portal *portals = malloc(sizeof(Portal));

    portals[0] = portal;

    room_set_portals(room, portals, 1);


    ck_assert_int_eq(room_get_start_position(room, &myX, &myY), OK);

    ck_assert_int_eq(myX, 3);
    ck_assert_int_eq(myY, 3);

}
END_TEST


START_TEST(test_room_get_start_position_null){

    int myX, myY;

    ck_assert_int_eq(room_get_start_position(NULL, &myX, &myY),INVALID_ARGUMENT);

}
END_TEST



START_TEST(test_room_get_id){
    ck_assert_int_eq(room_get_id(room), 1);
    ck_assert_int_eq(room_get_id(NULL), -1);
}
END_TEST

START_TEST(test_room_pick_up_treasure_success){
    Treasure *found = NULL;
    room_place_treasure(room, &treasure);
    ck_assert_int_eq(room_pick_up_treasure(room, treasure.id, &found), OK);
    ck_assert_ptr_nonnull(found);
    ck_assert(found->collected);
}
END_TEST

START_TEST(test_room_pick_up_treasure_not_found){
    Treasure *found = NULL;
    ck_assert_int_eq(room_pick_up_treasure(room, 999, &found), ROOM_NOT_FOUND);
    ck_assert_ptr_null(found);
}
END_TEST

START_TEST(test_room_pick_up_treasure_invalid_args){
    Treasure *found = NULL;
    room_place_treasure(room, &treasure);
    ck_assert_int_eq(room_pick_up_treasure(NULL, treasure.id, &found), INVALID_ARGUMENT);
    ck_assert_int_eq(room_pick_up_treasure(room, treasure.id, NULL), INVALID_ARGUMENT);
}
END_TEST

START_TEST(test_room_pick_up_treasure_already_collected){
    Treasure *found = NULL;
    room_place_treasure(room, &treasure);
    ck_assert_int_eq(room_pick_up_treasure(room, treasure.id, &found), OK);
    ck_assert_int_eq(room_pick_up_treasure(room, treasure.id, &found), INVALID_ARGUMENT);
}
END_TEST

START_TEST(test_destroy_treasure_null){
    destroy_treasure(NULL);
}
END_TEST

START_TEST(test_destroy_treasure_success){
    Treasure *t = malloc(sizeof(*t));
    t->name = strdup("foo");
    destroy_treasure(t);
}
END_TEST

START_TEST(test_room_is_walkable_pushable){
    // add a pushable in the center
    Pushable *p = malloc(sizeof(Pushable));
    p->id = 1;
    p->name = strdup("box");
    p->x = 2; p->y = 2;
    p->initial_x = 2; p->initial_y = 2;
    room->pushables = p;
    room->pushable_count = 1;
    ck_assert(room_is_walkable(room, 2, 2) == false);
    // cleanup
    free(p->name);
    free(room->pushables);
    room->pushables = NULL;
    room->pushable_count = 0;
    
}
END_TEST

START_TEST(test_room_has_pushable_at){

    //Add a pushable at 1,1 and check if it is detected by room_has_pushable_at
    Pushable *p = malloc(sizeof(Pushable));
    p->id = 2;
    p->name = strdup("bar");
    p->x = 1; p->y = 1;
    p->initial_x = 1; p->initial_y = 1;
    room->pushables = p;
    room->pushable_count = 1;
    int idx = -1;

    //Check that the pushable is found at 1,1 and that the index is correct
    ck_assert(room_has_pushable_at(room, 1, 1, &idx));
    ck_assert_int_eq(idx, 0);
    ck_assert(!room_has_pushable_at(room, 0, 0, NULL));
    free(p->name);
    free(room->pushables);
    room->pushables = NULL;
    room->pushable_count = 0;

}
END_TEST

START_TEST(test_room_try_push_success){

    // add a pushable at 1,1 and try to push it east into an empty tile
    Pushable *p = malloc(sizeof(Pushable));
    p->id = 3;
    p->name = strdup("push");
    p->x = 1; p->y = 1;
    p->initial_x = 1; p->initial_y = 1;
    room->pushables = p;
    room->pushable_count = 1;

    //Pushing east should succeed and move the pushable to 2,1
    ck_assert_int_eq(room_try_push(room, 0, DIR_EAST), OK);
    ck_assert_int_eq(room->pushables[0].x, 2);
    ck_assert_int_eq(room->pushables[0].y, 1);
    free(p->name);
    free(room->pushables);
    room->pushables = NULL;
    room->pushable_count = 0;

}
END_TEST

START_TEST(test_room_try_push_blocked){
    Pushable *p = malloc(sizeof(Pushable) * 2);
    // first pushable at 1,1
    p[0].id = 4;
    p[0].name = strdup("a");
    p[0].x = 1; p[0].y = 1;
    p[0].initial_x = 1; p[0].initial_y = 1;
    // second pushable blocking east
    p[1].id = 5;
    p[1].name = strdup("b");
    p[1].x = 2; p[1].y = 1;
    p[1].initial_x = 2; p[1].initial_y = 1;
    room->pushables = p;
    room->pushable_count = 2;

    //Trying to push the first pushable east should fail because of the second pushable
    ck_assert_int_eq(room_try_push(room, 0, DIR_EAST), ROOM_IMPASSABLE);
    ck_assert_int_eq(room_try_push(room, 0, DIR_NORTH), ROOM_IMPASSABLE); // wall
    ck_assert_int_eq(room_try_push(NULL, 0, DIR_NORTH), INVALID_ARGUMENT);
    ck_assert_int_eq(room_try_push(room, -1, DIR_NORTH), INVALID_ARGUMENT);
    ck_assert_int_eq(room_try_push(room, 0, (Direction)99), INVALID_ARGUMENT);
    free(p[0].name);
    free(p[1].name);
    free(room->pushables);
    room->pushables = NULL;
    room->pushable_count = 0;
}
END_TEST

START_TEST(test_room_try_push_consumes_on_switch){
    Pushable *p = malloc(sizeof(Pushable));
    p->id = 6;
    p->name = strdup("consume");
    p->x = 1;
    p->y = 1;
    p->initial_x = 1;
    p->initial_y = 1;
    room->pushables = p;
    room->pushable_count = 1;

    Switch *s = malloc(sizeof(Switch));
    s->id = 1;
    s->x = 2;
    s->y = 1;
    s->portal_id = 0;
    room->switches = s;
    room->switch_count = 1;

    ck_assert_int_eq(room_try_push(room, 0, DIR_EAST), OK);
    ck_assert_int_eq(room->pushables[0].x, -1);
    ck_assert_int_eq(room->pushables[0].y, -1);
    ck_assert(room_is_walkable(room, 2, 1) == true);

    free(p->name);
    free(room->pushables);
    room->pushables = NULL;
    room->pushable_count = 0;

    free(room->switches);
    room->switches = NULL;
    room->switch_count = 0;
}
END_TEST

START_TEST(test_room_is_walkable_switch_not_activating){
    Switch *s = malloc(sizeof(Switch));
    s->id = 2;
    s->x = 0;
    s->y = 0;
    s->portal_id = 0;
    room->switches = s;
    room->switch_count = 1;

    ck_assert(room_is_walkable(room, 0, 0) == true);

    free(room->switches);
    room->switches = NULL;
    room->switch_count = 0;
}
END_TEST





Suite *room_suite(void)
{
    Suite *s = suite_create("Room");
    TCase *tc = tcase_create("Basics");

    tcase_add_checked_fixture(tc, setup_room, teardown_room);

    tcase_add_test(tc, test_room_create_success);
    tcase_add_test(tc, test_room_create_null_name);
    
    tcase_add_test(tc, test_room_destroy_null);
    tcase_add_test(tc, test_room_destroy_with_contents);

    tcase_add_test(tc, test_room_get_width_success);
    tcase_add_test(tc, test_room_get_width_null);
    tcase_add_test(tc, test_room_get_height_success);
    tcase_add_test(tc, test_room_get_height_null);


    tcase_add_test(tc, test_room_is_walkable_true);
    tcase_add_test(tc, test_room_is_walkable_wall);
    tcase_add_test(tc, test_room_is_walkable_out_of_bounds);
    tcase_add_test(tc, test_room_is_walkable_null);
    

    tcase_add_test(tc, test_room_place_treasure_success);
    tcase_add_test(tc, test_room_place_treasure_null_room);
    tcase_add_test(tc, test_room_get_treasure_at_found);
    tcase_add_test(tc, test_room_get_treasure_at_not_found);
    tcase_add_test(tc, test_room_get_treasure_at_null);


    tcase_add_test(tc, test_room_get_portal_destination_found);
    tcase_add_test(tc, test_room_get_portal_destination_not_found);
    tcase_add_test(tc, test_room_get_portal_destination_null);


    tcase_add_test(tc, test_room_classify_tile_treasure);
    tcase_add_test(tc, test_room_classify_tile_portal);
    tcase_add_test(tc, test_room_classify_tile_wall);
    tcase_add_test(tc, test_room_classify_tile_null);


    tcase_add_test(tc, test_room_render_success);
    tcase_add_test(tc, test_room_render_null_room);


    tcase_add_test(tc, test_room_get_start_position_portal);
    tcase_add_test(tc, test_room_get_start_position_null);

    tcase_add_test(tc, test_room_try_push_blocked);
    tcase_add_test(tc, test_room_try_push_success);
    tcase_add_test(tc, test_room_try_push_consumes_on_switch);
    tcase_add_test(tc, test_room_is_walkable_switch_not_activating);

    tcase_add_test(tc, test_room_has_pushable_at);
    tcase_add_test(tc, test_room_is_walkable_pushable);
    
    tcase_add_test(tc, test_destroy_treasure_success);
    tcase_add_test(tc, test_destroy_treasure_null);

    tcase_add_test(tc, test_room_pick_up_treasure_already_collected);
    tcase_add_test(tc, test_room_pick_up_treasure_invalid_args);

    tcase_add_test(tc, test_room_pick_up_treasure_not_found);
    tcase_add_test(tc, test_room_pick_up_treasure_success);

    tcase_add_test(tc, test_room_get_id);

    suite_add_tcase(s, tc);

    return s;

}

