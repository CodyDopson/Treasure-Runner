#include <check.h>
#include <stdlib.h>
#include <string.h>

#include "game_engine.h"
#include "player.h"

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

    const Player *player = game_engine_get_player(engine);
    
    ck_assert_ptr_nonnull(player);

    ck_assert_int_ge(player->room_id, 0);
    ck_assert_int_ge(player->x, 0);
    ck_assert_int_ge(player->y, 0);

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

    const Player *player_before = game_engine_get_player(engine);
    int y_before = player_before->y;
    
    Status status = game_engine_move_player(engine, DIR_NORTH);
    
    const Player *player_after = game_engine_get_player(engine);
    
    if (status == OK) {
        ck_assert_int_eq(player_after->y, y_before - 1);
    }
}
END_TEST


START_TEST(test_game_engine_move_player_south){

    const Player *player_before = game_engine_get_player(engine);
    int y_before = player_before->y;
    
    Status status = game_engine_move_player(engine, DIR_SOUTH);
    
    const Player *player_after = game_engine_get_player(engine);
    
    if (status == OK) {
        ck_assert_int_eq(player_after->y, y_before + 1);
    }
}
END_TEST


START_TEST(test_game_engine_move_player_east){

    const Player *player_before = game_engine_get_player(engine);
    int x_before = player_before->x;
    
    Status status = game_engine_move_player(engine, DIR_EAST);
    
    const Player *player_after = game_engine_get_player(engine);
    
    if (status == OK) {
        ck_assert_int_eq(player_after->x, x_before + 1);
    }
}
END_TEST


START_TEST(test_game_engine_move_player_west){

    const Player *player_before = game_engine_get_player(engine);
    int x_before = player_before->x;
    
    Status status = game_engine_move_player(engine, DIR_WEST);
    
    const Player *player_after = game_engine_get_player(engine);
    
    if (status == OK) {
        ck_assert_int_eq(player_after->x, x_before - 1);
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

    const Player *player_before = game_engine_get_player(engine);
    int initial_room = player_before->room_id;
    int initial_x = player_before->x;
    int initial_y = player_before->y;
    
    // Move player
    game_engine_move_player(engine, DIR_NORTH);
    game_engine_move_player(engine, DIR_EAST);
    
    // Reset
    Status status = game_engine_reset(engine);
    ck_assert_int_eq(status, OK);
    
    const Player *player_after = game_engine_get_player(engine);
    ck_assert_int_eq(player_after->room_id, initial_room);
    ck_assert_int_eq(player_after->x, initial_x);
    ck_assert_int_eq(player_after->y, initial_y);
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
        const Player *player = game_engine_get_player(engine);
        room_id = player->room_id;
        
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
