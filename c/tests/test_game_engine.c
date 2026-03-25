#include <check.h>
#include <stdlib.h>
#include <string.h>

#include "game_engine.h"
#include "player.h"

/* Source-only game_engine accessors (implemented in c/src/game_engine.c). */
extern Status game_engine_get_player_room(const GameEngine *eng, int *room_out);
extern Status game_engine_get_player_position(const GameEngine *eng, int *x_out, int *y_out);
extern Status game_engine_get_player_collected_count(const GameEngine *eng, int *count_out);

static GameEngine *engine = NULL;
static const char *config_path = "../assets/starter.ini";



//Setup and teardown
static void setup_engine(void){
    
    Status status = game_engine_create(config_path, &engine);

    ck_assert_int_eq(status, OK);
    ck_assert_ptr_nonnull(engine);

}

static void teardown_engine(void){
    
    game_engine_destroy(engine);
    engine = NULL;

}




//Tests for create
START_TEST(test_game_engine_create_success){

    GameEngine *eng = NULL;
    Status status = game_engine_create(config_path, &eng);
    
    ck_assert_int_eq(status, OK);
    ck_assert_ptr_nonnull(eng);
    ck_assert_ptr_nonnull(eng->graph);
    ck_assert_ptr_nonnull(eng->player);
    ck_assert(eng->room_count > 0);
    
    game_engine_destroy(eng);
}
END_TEST


START_TEST(test_game_engine_create_null_config){

    GameEngine *eng = NULL;
    Status status = game_engine_create(NULL, &eng);
    
    ck_assert_int_eq(status, INVALID_ARGUMENT);
    ck_assert_ptr_null(eng);
}
END_TEST


START_TEST(test_game_engine_create_null_engine_out){

    Status status = game_engine_create(config_path, NULL);
    
    ck_assert_int_eq(status, INVALID_ARGUMENT);
}
END_TEST


START_TEST(test_game_engine_create_invalid_config){

    GameEngine *eng = NULL;
    Status status = game_engine_create("nonexistent/invalid/config.ini", &eng);
    
    ck_assert_int_ne(status, OK);
}
END_TEST


START_TEST(test_game_engine_destroy_null){

    game_engine_destroy(NULL);
}
END_TEST


START_TEST(test_game_engine_destroy_success){

    GameEngine *eng = NULL;
    game_engine_create(config_path, &eng);
    ck_assert_ptr_nonnull(eng);
    
    game_engine_destroy(eng);
}
END_TEST


/* ============================================================
 * Player Access Tests
 * ============================================================ */

START_TEST(test_game_engine_get_player_success){

    int room_id = -1;
    int x = -1;
    int y = -1;
    int collected = -1;

    ck_assert_int_eq(game_engine_get_player_room(engine, &room_id), OK);
    ck_assert_int_eq(game_engine_get_player_position(engine, &x, &y), OK);
    ck_assert_int_eq(game_engine_get_player_collected_count(engine, &collected), OK);

    ck_assert_int_ge(room_id, 0);
    ck_assert_int_ge(x, 0);
    ck_assert_int_ge(y, 0);
    ck_assert_int_ge(collected, 0);

}
END_TEST


START_TEST(test_game_engine_get_player_null){

    const Player *player = game_engine_get_player(NULL);
    
    ck_assert_ptr_null(player);
}
END_TEST


/* ============================================================
 * Movement Tests
 * ============================================================ */

START_TEST(test_game_engine_move_player_north){

    int x_before = 0;
    int y_before = 0;
    ck_assert_int_eq(game_engine_get_player_position(engine, &x_before, &y_before), OK);
    
    Status status = game_engine_move_player(engine, DIR_NORTH);
    int x_after = 0;
    int y_after = 0;
    ck_assert_int_eq(game_engine_get_player_position(engine, &x_after, &y_after), OK);
    
    if (status == OK) {
        ck_assert_int_eq(y_after, y_before - 1);
    }
}
END_TEST


START_TEST(test_game_engine_move_player_south){

    int x_before = 0;
    int y_before = 0;
    ck_assert_int_eq(game_engine_get_player_position(engine, &x_before, &y_before), OK);
    
    Status status = game_engine_move_player(engine, DIR_SOUTH);
    int x_after = 0;
    int y_after = 0;
    ck_assert_int_eq(game_engine_get_player_position(engine, &x_after, &y_after), OK);
    
    if (status == OK) {
        ck_assert_int_eq(y_after, y_before + 1);
    }
}
END_TEST


START_TEST(test_game_engine_move_player_east){

    int x_before = 0;
    int y_before = 0;
    ck_assert_int_eq(game_engine_get_player_position(engine, &x_before, &y_before), OK);
    
    Status status = game_engine_move_player(engine, DIR_EAST);
    int x_after = 0;
    int y_after = 0;
    ck_assert_int_eq(game_engine_get_player_position(engine, &x_after, &y_after), OK);
    
    if (status == OK) {
        ck_assert_int_eq(x_after, x_before + 1);
    }
}
END_TEST


START_TEST(test_game_engine_move_player_west){

    int x_before = 0;
    int y_before = 0;
    ck_assert_int_eq(game_engine_get_player_position(engine, &x_before, &y_before), OK);
    
    Status status = game_engine_move_player(engine, DIR_WEST);
    int x_after = 0;
    int y_after = 0;
    ck_assert_int_eq(game_engine_get_player_position(engine, &x_after, &y_after), OK);
    
    if (status == OK) {
        ck_assert_int_eq(x_after, x_before - 1);
    }
}
END_TEST


START_TEST(test_game_engine_move_player_null_engine){

    Status status = game_engine_move_player(NULL, DIR_NORTH);
    
    ck_assert_int_eq(status, INVALID_ARGUMENT);
}
END_TEST


START_TEST(test_game_engine_move_player_invalid_direction){

    // Try an invalid direction (assuming 4 is out of range)
    Status status = game_engine_move_player(engine, (Direction)99);
    
    ck_assert_int_eq(status, INVALID_ARGUMENT);
}
END_TEST


/* ============================================================
 * Room Count Query Tests
 * ============================================================ */

START_TEST(test_game_engine_get_room_count_success){

    int count = 0;
    Status status = game_engine_get_room_count(engine, &count);
    
    ck_assert_int_eq(status, OK);
    ck_assert(count > 0);
}
END_TEST


START_TEST(test_game_engine_get_room_count_null_engine){

    int count = 0;
    Status status = game_engine_get_room_count(NULL, &count);
    
    ck_assert_int_eq(status, INVALID_ARGUMENT);
}
END_TEST


START_TEST(test_game_engine_get_room_count_null_count_out){

    Status status = game_engine_get_room_count(engine, NULL);
    
    ck_assert_int_eq(status, NULL_POINTER);
}
END_TEST


/* ============================================================
 * Room Dimensions Query Tests
 * ============================================================ */

START_TEST(test_game_engine_get_room_dimensions_success){

    int width = 0, height = 0;
    Status status = game_engine_get_room_dimensions(engine, &width, &height);
    
    ck_assert_int_eq(status, OK);
    ck_assert(width > 0);
    ck_assert(height > 0);
}
END_TEST


START_TEST(test_game_engine_get_room_dimensions_null_engine){

    int width = 0, height = 0;
    Status status = game_engine_get_room_dimensions(NULL, &width, &height);
    
    ck_assert_int_eq(status, INVALID_ARGUMENT);
}
END_TEST


START_TEST(test_game_engine_get_room_dimensions_null_width){

    int height = 0;
    Status status = game_engine_get_room_dimensions(engine, NULL, &height);
    
    ck_assert_int_eq(status, NULL_POINTER);
}
END_TEST


START_TEST(test_game_engine_get_room_dimensions_null_height){

    int width = 0;
    Status status = game_engine_get_room_dimensions(engine, &width, NULL);
    
    ck_assert_int_eq(status, NULL_POINTER);
}
END_TEST


START_TEST(test_game_engine_get_room_dimensions_both_null){

    Status status = game_engine_get_room_dimensions(engine, NULL, NULL);
    
    ck_assert_int_eq(status, NULL_POINTER);
}
END_TEST


/* ============================================================
 * Reset Tests
 * ============================================================ */

START_TEST(test_game_engine_reset_success){

    int initial_room = -1;
    int initial_x = -1;
    int initial_y = -1;
    ck_assert_int_eq(game_engine_get_player_room(engine, &initial_room), OK);
    ck_assert_int_eq(game_engine_get_player_position(engine, &initial_x, &initial_y), OK);
    
    // Move player
    game_engine_move_player(engine, DIR_NORTH);
    game_engine_move_player(engine, DIR_EAST);
    
    // Reset
    Status status = game_engine_reset(engine);
    ck_assert_int_eq(status, OK);
    
    int room_after = -1;
    int x_after = -1;
    int y_after = -1;
    ck_assert_int_eq(game_engine_get_player_room(engine, &room_after), OK);
    ck_assert_int_eq(game_engine_get_player_position(engine, &x_after, &y_after), OK);
    ck_assert_int_eq(room_after, initial_room);
    ck_assert_int_eq(x_after, initial_x);
    ck_assert_int_eq(y_after, initial_y);
}
END_TEST


START_TEST(test_game_engine_reset_null_engine){

    Status status = game_engine_reset(NULL);
    
    ck_assert_int_eq(status, INVALID_ARGUMENT);
}
END_TEST


/* ============================================================
 * Render Tests
 * ============================================================ */

START_TEST(test_game_engine_render_current_room_success){

    char *str = NULL;
    Status status = game_engine_render_current_room(engine, &str);
    
    ck_assert_int_eq(status, OK);
    ck_assert_ptr_nonnull(str);
    ck_assert(strlen(str) > 0);
    
    free(str);
}
END_TEST


START_TEST(test_game_engine_render_current_room_null_engine){

    char *str = NULL;
    Status status = game_engine_render_current_room(NULL, &str);
    
    ck_assert_int_eq(status, INVALID_ARGUMENT);
    ck_assert_ptr_null(str);
}
END_TEST


START_TEST(test_game_engine_render_current_room_null_str_out){

    Status status = game_engine_render_current_room(engine, NULL);
    
    ck_assert_int_eq(status, INVALID_ARGUMENT);
}
END_TEST


START_TEST(test_game_engine_render_room_success){

    int room_id = 0;
    int count = 0;
    game_engine_get_room_count(engine, &count);
    
    if (count > 0) {
        ck_assert_int_eq(game_engine_get_player_room(engine, &room_id), OK);
        
        char *str = NULL;
        Status status = game_engine_render_room(engine, room_id, &str);
        
        ck_assert_int_eq(status, OK);
        ck_assert_ptr_nonnull(str);
        ck_assert(strlen(str) > 0);
        
        free(str);
    }
}
END_TEST


START_TEST(test_game_engine_render_room_null_engine){

    char *str = NULL;
    Status status = game_engine_render_room(NULL, 0, &str);
    
    ck_assert_int_eq(status, INVALID_ARGUMENT);
    ck_assert_ptr_null(str);
}
END_TEST


START_TEST(test_game_engine_render_room_null_str_out){

    Status status = game_engine_render_room(engine, 0, NULL);
    
    ck_assert_int_eq(status, NULL_POINTER);
}
END_TEST


START_TEST(test_game_engine_render_room_invalid_room_id){

    char *str = NULL;
    Status status = game_engine_render_room(engine, 99999, &str);
    
    ck_assert_int_eq(status, GE_NO_SUCH_ROOM);
    ck_assert_ptr_null(str);

}
END_TEST



//Room ID Tests
START_TEST(test_game_engine_get_room_ids_success){

    int *ids = NULL;
    int count = 0;
    Status status = game_engine_get_room_ids(engine, &ids, &count);
    
    ck_assert_int_eq(status, OK);
    ck_assert_ptr_nonnull(ids);
    ck_assert(count > 0);
    
    free(ids);

}
END_TEST


START_TEST(test_game_engine_get_room_ids_null_engine){

    int *ids = NULL;
    int count = 0;
    Status status = game_engine_get_room_ids(NULL, &ids, &count);
    
    ck_assert_int_eq(status, INVALID_ARGUMENT);
    ck_assert_ptr_null(ids);

}
END_TEST



START_TEST(test_game_engine_get_room_ids_null_ids_out){

    int count = 0;
    Status status = game_engine_get_room_ids(engine, NULL, &count);
    
    ck_assert_int_eq(status, NULL_POINTER);

}
END_TEST



START_TEST(test_game_engine_get_room_ids_null_count_out){

    int *ids = NULL;
    Status status = game_engine_get_room_ids(engine, &ids, NULL);
    
    ck_assert_int_eq(status, NULL_POINTER);

}
END_TEST



START_TEST(test_game_engine_get_room_ids_both_null){

    Status status = game_engine_get_room_ids(engine, NULL, NULL);
    
    ck_assert_int_eq(status, NULL_POINTER);

}
END_TEST





Suite *game_engine_suite(void){

    Suite *s = suite_create("GameEngine");
    
    TCase *tc_creation = tcase_create("Creation");
    
    tcase_add_test(tc_creation, test_game_engine_create_success);
    tcase_add_test(tc_creation, test_game_engine_create_null_config);
    tcase_add_test(tc_creation, test_game_engine_create_null_engine_out);
    tcase_add_test(tc_creation, test_game_engine_create_invalid_config);

    tcase_add_test(tc_creation, test_game_engine_destroy_null);
    tcase_add_test(tc_creation, test_game_engine_destroy_success);
    suite_add_tcase(s, tc_creation);
    

    TCase *tc_player = tcase_create("PlayerAccess");
    tcase_add_checked_fixture(tc_player, setup_engine, teardown_engine);

    tcase_add_test(tc_player, test_game_engine_get_player_success);
    tcase_add_test(tc_player, test_game_engine_get_player_null);

    suite_add_tcase(s, tc_player);
    

    TCase *tc_movement = tcase_create("Movement");
    tcase_add_checked_fixture(tc_movement, setup_engine, teardown_engine);

    tcase_add_test(tc_movement, test_game_engine_move_player_north);
    tcase_add_test(tc_movement, test_game_engine_move_player_south);
    tcase_add_test(tc_movement, test_game_engine_move_player_east);
    tcase_add_test(tc_movement, test_game_engine_move_player_west);

    tcase_add_test(tc_movement, test_game_engine_move_player_null_engine);
    tcase_add_test(tc_movement, test_game_engine_move_player_invalid_direction);
    suite_add_tcase(s, tc_movement);
    

    TCase *tc_queries = tcase_create("Queries");
    tcase_add_checked_fixture(tc_queries, setup_engine, teardown_engine);

    tcase_add_test(tc_queries, test_game_engine_get_room_count_success);
    tcase_add_test(tc_queries, test_game_engine_get_room_count_null_engine);
    tcase_add_test(tc_queries, test_game_engine_get_room_count_null_count_out);

    tcase_add_test(tc_queries, test_game_engine_get_room_dimensions_success);
    tcase_add_test(tc_queries, test_game_engine_get_room_dimensions_null_engine);
    tcase_add_test(tc_queries, test_game_engine_get_room_dimensions_null_width);
    tcase_add_test(tc_queries, test_game_engine_get_room_dimensions_null_height);
    tcase_add_test(tc_queries, test_game_engine_get_room_dimensions_both_null);


    tcase_add_test(tc_queries, test_game_engine_get_room_ids_success);
    tcase_add_test(tc_queries, test_game_engine_get_room_ids_null_engine);
    tcase_add_test(tc_queries, test_game_engine_get_room_ids_null_ids_out);
    tcase_add_test(tc_queries, test_game_engine_get_room_ids_null_count_out);
    tcase_add_test(tc_queries, test_game_engine_get_room_ids_both_null);
    suite_add_tcase(s, tc_queries);
    

    TCase *tc_reset = tcase_create("Reset");
    tcase_add_checked_fixture(tc_reset, setup_engine, teardown_engine);

    tcase_add_test(tc_reset, test_game_engine_reset_success);
    tcase_add_test(tc_reset, test_game_engine_reset_null_engine);

    suite_add_tcase(s, tc_reset);
    

    TCase *tc_render = tcase_create("Rendering");
    tcase_add_checked_fixture(tc_render, setup_engine, teardown_engine);

    tcase_add_test(tc_render, test_game_engine_render_current_room_success);
    tcase_add_test(tc_render, test_game_engine_render_current_room_null_engine);
    tcase_add_test(tc_render, test_game_engine_render_current_room_null_str_out);

    tcase_add_test(tc_render, test_game_engine_render_room_success);
    tcase_add_test(tc_render, test_game_engine_render_room_null_engine);
    tcase_add_test(tc_render, test_game_engine_render_room_null_str_out);
    tcase_add_test(tc_render, test_game_engine_render_room_invalid_room_id);

    suite_add_tcase(s, tc_render);
    
    return s;
}
