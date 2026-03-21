#include <check.h>
#include <stdlib.h>
#include <string.h>

#include "world_loader.h"
#include "graph.h"
#include "room.h"
#include "datagen.h"

static const char *config_path = "../assets/starter.ini";


//Setup and teardown
static void setup_world_loader(void){ 
    
    //Doesn't do anything

}

static void teardown_world_loader(void){

    /* Ensure any datagen state is cleaned up between tests */
    stop_datagen();
    
}


/* ============================================================
 * Tests for loader_load_world
 * ============================================================ */

START_TEST(test_loader_load_world_success){

    Graph *graph = NULL;
    Room *first_room = NULL;
    int num_rooms = 0;
    Charset charset;
    
    Status status = loader_load_world(config_path, &graph, &first_room, 
                                      &num_rooms, &charset);
    
    ck_assert_int_eq(status, OK);
    ck_assert_ptr_nonnull(graph);
    ck_assert_ptr_nonnull(first_room);
    ck_assert(num_rooms > 0);
    
    // Verify charset was populated
    ck_assert(charset.wall != '\0' || charset.floor != '\0');
    
    // Cleanup
    graph_destroy(graph);
}
END_TEST


START_TEST(test_loader_load_world_null_config_file){

    Graph *graph = NULL;
    Room *first_room = NULL;
    int num_rooms = 0;
    Charset charset;
    
    Status status = loader_load_world(NULL, &graph, &first_room, 
                                      &num_rooms, &charset);
    
    ck_assert_int_eq(status, INVALID_ARGUMENT);
    ck_assert_ptr_null(graph);
    ck_assert_ptr_null(first_room);
}
END_TEST


START_TEST(test_loader_load_world_null_graph_out){

    Room *first_room = NULL;
    int num_rooms = 0;
    Charset charset;
    
    Status status = loader_load_world(config_path, NULL, &first_room, 
                                      &num_rooms, &charset);
    
    ck_assert_int_eq(status, INVALID_ARGUMENT);
}
END_TEST


START_TEST(test_loader_load_world_null_first_room_out){

    Graph *graph = NULL;
    int num_rooms = 0;
    Charset charset;
    
    Status status = loader_load_world(config_path, &graph, NULL, 
                                      &num_rooms, &charset);
    
    ck_assert_int_eq(status, INVALID_ARGUMENT);
}
END_TEST


START_TEST(test_loader_load_world_null_num_rooms_out){

    Graph *graph = NULL;
    Room *first_room = NULL;
    Charset charset;
    
    Status status = loader_load_world(config_path, &graph, &first_room, 
                                      NULL, &charset);
    
    ck_assert_int_eq(status, INVALID_ARGUMENT);
}
END_TEST


START_TEST(test_loader_load_world_null_charset_out){

    Graph *graph = NULL;
    Room *first_room = NULL;
    int num_rooms = 0;
    
    Status status = loader_load_world(config_path, &graph, &first_room, 
                                      &num_rooms, NULL);
    
    ck_assert_int_eq(status, INVALID_ARGUMENT);
}
END_TEST


START_TEST(test_loader_load_world_null_graph_and_first_room){

    int num_rooms = 0;
    Charset charset;
    
    Status status = loader_load_world(config_path, NULL, NULL, 
                                      &num_rooms, &charset);
    
    ck_assert_int_eq(status, INVALID_ARGUMENT);
}
END_TEST


START_TEST(test_loader_load_world_null_num_rooms_and_charset){

    Graph *graph = NULL;
    Room *first_room = NULL;
    
    Status status = loader_load_world(config_path, &graph, &first_room, 
                                      NULL, NULL);
    
    ck_assert_int_eq(status, INVALID_ARGUMENT);
}
END_TEST


START_TEST(test_loader_load_world_all_null_outputs){

    Status status = loader_load_world(config_path, NULL, NULL, 
                                      NULL, NULL);
    
    ck_assert_int_eq(status, INVALID_ARGUMENT);
}
END_TEST


START_TEST(test_loader_load_world_invalid_config_file){

    Graph *graph = NULL;
    Room *first_room = NULL;
    int num_rooms = 0;
    Charset charset;
    
    Status status = loader_load_world("nonexistent/invalid/config.ini", 
                                      &graph, &first_room, 
                                      &num_rooms, &charset);
    
    ck_assert_int_ne(status, OK);
    ck_assert(status == WL_ERR_CONFIG || status == WL_ERR_DATAGEN);
}
END_TEST


START_TEST(test_loader_load_world_invalid_config_empty_string){

    Graph *graph = NULL;
    Room *first_room = NULL;
    int num_rooms = 0;
    Charset charset;
    
    Status status = loader_load_world("", &graph, &first_room, 
                                      &num_rooms, &charset);
    
    ck_assert_int_ne(status, OK);
}
END_TEST


START_TEST(test_loader_load_world_graph_contains_rooms){

    Graph *graph = NULL;
    Room *first_room = NULL;
    int num_rooms = 0;
    Charset charset;
    
    Status status = loader_load_world(config_path, &graph, &first_room, 
                                      &num_rooms, &charset);
    
    ck_assert_int_eq(status, OK);
    ck_assert_ptr_nonnull(graph);
    ck_assert_ptr_nonnull(first_room);
    ck_assert(num_rooms > 0);
    
    // Verify first room is in the graph
    bool contains = graph_contains(graph, first_room);
    ck_assert(contains);
    
    // Verify graph size matches num_rooms
    int graph_size_result = graph_size(graph);
    ck_assert_int_eq(graph_size_result, num_rooms);
    
    graph_destroy(graph);
}
END_TEST


START_TEST(test_loader_load_world_first_room_valid){

    Graph *graph = NULL;
    Room *first_room = NULL;
    int num_rooms = 0;
    Charset charset;
    
    Status status = loader_load_world(config_path, &graph, &first_room, 
                                      &num_rooms, &charset);
    
    ck_assert_int_eq(status, OK);
    ck_assert_ptr_nonnull(first_room);
    
    // Verify first room has valid dimensions
    int width = room_get_width(first_room);
    int height = room_get_height(first_room);
    
    ck_assert(width > 0);
    ck_assert(height > 0);
    
    graph_destroy(graph);
}
END_TEST


START_TEST(test_loader_load_world_charset_populated){

    Graph *graph = NULL;
    Room *first_room = NULL;
    int num_rooms = 0;
    Charset charset;
    
    Status status = loader_load_world(config_path, &graph, &first_room, 
                                      &num_rooms, &charset);
    
    ck_assert_int_eq(status, OK);
    
    // Verify charset fields are set (at least some of them)
    ck_assert(charset.wall != '\0' || 
              charset.floor != '\0' || 
              charset.player != '\0' ||
              charset.treasure != '\0' ||
              charset.portal != '\0');
    
    graph_destroy(graph);
}
END_TEST


START_TEST(test_loader_load_world_num_rooms_matches_graph_size){

    Graph *graph = NULL;
    Room *first_room = NULL;
    int num_rooms = 0;
    Charset charset;
    
    Status status = loader_load_world(config_path, &graph, &first_room, 
                                      &num_rooms, &charset);
    
    ck_assert_int_eq(status, OK);
    
    int graph_size_result = graph_size(graph);
    ck_assert_int_eq(num_rooms, graph_size_result);
    
    graph_destroy(graph);
}
END_TEST


START_TEST(test_loader_load_world_multiple_loads){

    Graph *graph1 = NULL, *graph2 = NULL;
    Room *first_room1 = NULL, *first_room2 = NULL;
    int num_rooms1 = 0, num_rooms2 = 0;
    Charset charset1, charset2;
    
    Status status1 = loader_load_world(config_path, &graph1, &first_room1, 
                                       &num_rooms1, &charset1);
    Status status2 = loader_load_world(config_path, &graph2, &first_room2, 
                                       &num_rooms2, &charset2);
    
    ck_assert_int_eq(status1, OK);
    ck_assert_int_eq(status2, OK);
    ck_assert_int_eq(num_rooms1, num_rooms2);
    
    // Cleanup both graphs
    graph_destroy(graph1);
    graph_destroy(graph2);
}
END_TEST


START_TEST(test_loader_load_world_graph_reusable){

    Graph *graph = NULL;
    Room *first_room = NULL;
    int num_rooms = 0;
    Charset charset;
    
    Status status = loader_load_world(config_path, &graph, &first_room, 
                                      &num_rooms, &charset);
    
    ck_assert_int_eq(status, OK);
    
    // Verify we can query the graph
    int size = graph_size(graph);
    ck_assert_int_eq(size, num_rooms);
    
    // Verify the graph contains the first room
    bool contains = graph_contains(graph, first_room);
    ck_assert(contains);
    
    graph_destroy(graph);
}
END_TEST


/* ============================================================
 * Suite Definition
 * ============================================================ */

Suite *world_loader_suite(void){
    Suite *s = suite_create("WorldLoader");
    
    TCase *tc_core = tcase_create("Core");
    tcase_add_checked_fixture(tc_core, setup_world_loader, teardown_world_loader);
    
    // Success and basic tests
    tcase_add_test(tc_core, test_loader_load_world_success);
    tcase_add_test(tc_core, test_loader_load_world_graph_contains_rooms);
    tcase_add_test(tc_core, test_loader_load_world_first_room_valid);
    tcase_add_test(tc_core, test_loader_load_world_charset_populated);
    tcase_add_test(tc_core, test_loader_load_world_num_rooms_matches_graph_size);
    
    // Null pointer tests
    tcase_add_test(tc_core, test_loader_load_world_null_config_file);
    tcase_add_test(tc_core, test_loader_load_world_null_graph_out);
    tcase_add_test(tc_core, test_loader_load_world_null_first_room_out);
    tcase_add_test(tc_core, test_loader_load_world_null_num_rooms_out);
    tcase_add_test(tc_core, test_loader_load_world_null_charset_out);
    tcase_add_test(tc_core, test_loader_load_world_null_graph_and_first_room);
    tcase_add_test(tc_core, test_loader_load_world_null_num_rooms_and_charset);
    tcase_add_test(tc_core, test_loader_load_world_all_null_outputs);
    
    // Invalid config tests
    tcase_add_test(tc_core, test_loader_load_world_invalid_config_file);
    tcase_add_test(tc_core, test_loader_load_world_invalid_config_empty_string);
    
    // Edge cases and integration tests
    tcase_add_test(tc_core, test_loader_load_world_multiple_loads);
    tcase_add_test(tc_core, test_loader_load_world_graph_reusable);
    
    suite_add_tcase(s, tc_core);
    
    return s;
}
