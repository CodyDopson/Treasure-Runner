#include <check.h>
#include <stdlib.h>

#include "player.h"

static Player *play = NULL;

//Setup and teardown
static void setup_player(void){

    Status status = player_create(1,0,0,&play);
    ck_assert_int_eq(status, OK);
    ck_assert_ptr_nonnull(play);

}

static void teardown_player(void){

    player_destroy(play);
    play = NULL;

}


//Create player tests
START_TEST(test_player_create_success){

    Player *myP = NULL;

    ck_assert_int_eq(player_create(1, 2, 3, &myP), OK);
    ck_assert_ptr_nonnull(myP);

    ck_assert_int_eq(myP->room_id, 1);
    ck_assert_int_eq(myP->x, 2);
    ck_assert_int_eq(myP->y, 3);

    player_destroy(myP);

}
END_TEST



START_TEST(test_player_create_null){

    ck_assert_int_eq(player_create(1, 2, 3, NULL), INVALID_ARGUMENT);

}
END_TEST



//Destroy player tests
START_TEST(test_player_destroy_success){


    Player *myP = NULL;

    ck_assert_int_eq(player_create(1, 2, 3, &myP), OK);
    ck_assert_ptr_nonnull(myP);
    player_destroy(myP);

}
END_TEST




START_TEST(test_player_destroy_null){

    player_destroy(NULL);

}
END_TEST


//Get room tests
START_TEST(test_player_get_room_success){

    Player *myP = NULL;

    player_create(5, 0, 0, &myP);
    ck_assert_int_eq(player_get_room(myP), 5);
    player_destroy(myP);

}
END_TEST




START_TEST(test_player_get_room_null){

    ck_assert_int_eq(player_get_room(NULL), -1);

}
END_TEST



//Get positions tests
START_TEST(test_player_get_position_success){

    int myX, myY;

    Player *myP = NULL;
    player_create(1, 4, 5, &myP);

    ck_assert_int_eq(player_get_position(myP, &myX, &myY), OK);
    ck_assert_int_eq(myX, 4);
    ck_assert_int_eq(myY, 5);

    player_destroy(myP);
    
}
END_TEST




START_TEST(test_player_get_position_invalid_args){

    int myX, myY;

    ck_assert_int_eq(player_get_position(NULL, &myX, &myY), INVALID_ARGUMENT);
    ck_assert_int_eq(player_get_position(play, NULL, &myY), INVALID_ARGUMENT);
    ck_assert_int_eq(player_get_position(play, &myX, NULL), INVALID_ARGUMENT);

}
END_TEST


//Set position tests
START_TEST(test_player_set_position_success){

    int myX, myY;

    Player *myP = NULL;
    player_create(1, 0, 0, &myP);

    ck_assert_int_eq(player_set_position(myP, 7, 8), OK);
    ck_assert_int_eq(player_get_position(myP, &myX, &myY), OK);

    ck_assert_int_eq(myX, 7);
    ck_assert_int_eq(myY, 8);

    player_destroy(myP);

}
END_TEST




START_TEST(test_player_set_position_null){

    ck_assert_int_eq(player_set_position(NULL, 1, 1), INVALID_ARGUMENT);
    
}
END_TEST



//Move tests
START_TEST(test_player_move_to_room_success){


    Player *myP = NULL;
    player_create(1, 2, 3, &myP);

    ck_assert_int_eq(player_move_to_room(myP, 2), OK);
    ck_assert_int_eq(player_get_room(myP), 2);

    //check unchanged
    ck_assert_int_eq(myP->x, 2);
    ck_assert_int_eq(myP->y, 3);

    player_destroy(myP);


}
END_TEST

START_TEST(test_player_move_to_room_null){

    ck_assert_int_eq(player_move_to_room(NULL, 5), INVALID_ARGUMENT);

}
END_TEST




//Reset to start tests
START_TEST(test_player_reset_to_start_success){


    Player *myP = NULL;
    player_create(10, 5, 5, &myP);

    ck_assert_int_eq(player_reset_to_start(myP, 1, 0, 0), OK);

    ck_assert_int_eq(myP->room_id, 1);
    ck_assert_int_eq(myP->x, 0);
    ck_assert_int_eq(myP->y, 0);

    player_destroy(myP);

}
END_TEST

START_TEST(test_player_reset_to_start_null){

    ck_assert_int_eq(player_reset_to_start(NULL, 1, 0, 0),INVALID_ARGUMENT);

}
END_TEST


Suite *player_suite(void){

    Suite *s = suite_create("Player");

    TCase *tc = tcase_create("Basics");

    tcase_add_checked_fixture(tc, setup_player, teardown_player);

    tcase_add_test(tc, test_player_create_success);
    tcase_add_test(tc, test_player_create_null);

    tcase_add_test(tc, test_player_destroy_success);
    tcase_add_test(tc, test_player_destroy_null);

    tcase_add_test(tc, test_player_get_room_success);
    tcase_add_test(tc, test_player_get_room_null);

    tcase_add_test(tc, test_player_get_position_success);
    tcase_add_test(tc, test_player_get_position_invalid_args);

    tcase_add_test(tc, test_player_set_position_success);
    tcase_add_test(tc, test_player_set_position_null);

    tcase_add_test(tc, test_player_move_to_room_success);
    tcase_add_test(tc, test_player_move_to_room_null);

    tcase_add_test(tc, test_player_reset_to_start_success);
    tcase_add_test(tc, test_player_reset_to_start_null);

    suite_add_tcase(s, tc);

    return s;

}
