#ifndef GAME_ENGINE_ACCESSORS_H
#define GAME_ENGINE_ACCESSORS_H

#include "types.h"
#include <stdbool.h>

/* Forward declaration */
typedef struct GameEngine GameEngine;

/*
 * Accessor-side engine state used to store metadata that must not be added
 * directly to GameEngine in game_engine.h.
 */
typedef struct GameEngineAccessorState {
    GameEngine *engine;                    /* Owning engine instance */
    int total_treasure_count;              /* Cached world treasure total */
    bool is_game_over;                     /* Terminal-state flag */
    bool is_victory;                       /* Completion-state flag */
    struct GameEngineAccessorState *next;  /* Registry linkage */
} GameEngineAccessorState;

/* ============================================================
 * Player State Accessors
 * 
 * These functions allow external code (e.g., Python views)
 * to query player state through the GameEngine, maintaining
 * the MVC pattern where the View queries the Controller,
 * not the Model directly.
 * ============================================================ */

/*
 * game_engine_get_player_room
 * ---------------------------
 * Get the current room ID where the player is located.
 *
 * Parameters:
 *   eng:      The game engine
 *   room_out: Output pointer for the room ID
 *
 * Returns:
 *   OK on success
 *   INVALID_ARGUMENT if eng is NULL
 *   NULL_POINTER if room_out is NULL
 *   INTERNAL_ERROR if player or room is invalid
 */
Status game_engine_get_player_room(const GameEngine *eng, int *room_out);

/*
 * game_engine_get_player_position
 * --------------------------------
 * Get the player's (x, y) position in their current room.
 *
 * Parameters:
 *   eng:    The game engine
 *   x_out:  Output pointer for x coordinate
 *   y_out:  Output pointer for y coordinate
 *
 * Returns:
 *   OK on success
 *   INVALID_ARGUMENT if eng is NULL
 *   NULL_POINTER if x_out or y_out is NULL
 *   INTERNAL_ERROR if player is invalid
 */
Status game_engine_get_player_position(const GameEngine *eng,
                                       int *x_out,
                                       int *y_out);

/*
 * game_engine_get_player_collected_count
 * ----------------------------------------
 * Get the number of treasures the player has collected.
 *
 * Parameters:
 *   eng:       The game engine
 *   count_out: Output pointer for treasure count
 *
 * Returns:
 *   OK on success
 *   INVALID_ARGUMENT if eng is NULL
 *   NULL_POINTER if count_out is NULL
 *   INTERNAL_ERROR if player is invalid
 */
Status game_engine_get_player_collected_count(const GameEngine *eng,
                                              int *count_out);

/*
 * game_engine_get_total_treasure_count
 * ------------------------------------
 * Get the total number of treasures in the world.
 *
 * Parameters:
 *   eng:       The game engine
 *   count_out: Output pointer for total treasure count
 *
 * Returns:
 *   OK on success
 *   INVALID_ARGUMENT if eng is NULL
 *   NULL_POINTER if count_out is NULL
 */
Status game_engine_get_total_treasure_count(const GameEngine *eng,
                                            int *count_out);

/*
 * game_engine_is_game_over
 * ------------------------
 * Check whether the game has reached a terminal state.
 *
 * Returns:
 *   OK on success (is_over_out is set)
 *   INVALID_ARGUMENT if eng is NULL
 *   NULL_POINTER if is_over_out is NULL
 */
Status game_engine_is_game_over(const GameEngine *eng,
                                bool *is_over_out);

/*
 * game_engine_is_victory
 * ----------------------
 * Check whether the game is in a victory state.
 *
 * Returns:
 *   OK on success (is_victory_out is set)
 *   INVALID_ARGUMENT if eng is NULL
 *   NULL_POINTER if is_victory_out is NULL
 */
Status game_engine_is_victory(const GameEngine *eng,
                              bool *is_victory_out);

/*
 * game_engine_player_has_collected_treasure
 * -------------------------------------------
 * Check if the player has collected a specific treasure.
 *
 * Parameters:
 *   eng:        The game engine
 *   treasure_id: ID of the treasure to check
 *   has_out:    Output pointer for boolean result
 *
 * Returns:
 *   OK on success (has_out is set to true/false)
 *   INVALID_ARGUMENT if eng is NULL or treasure_id is invalid
 *   NULL_POINTER if has_out is NULL
 *   INTERNAL_ERROR if player is invalid
 */
Status game_engine_player_has_collected_treasure(const GameEngine *eng,
                                                  int treasure_id,
                                                  bool *has_out);

/*
 * game_engine_get_player_collected_treasures
 * -------------------------------------------
 * Get all treasures that the player has collected.
 *
 * Parameters:
 *   eng:              The game engine
 *   treasures_out:    Output pointer for treasure array
 *   count_out:        Output pointer for array length
 *
 * Returns:
 *   OK on success
 *   INVALID_ARGUMENT if eng is NULL
 *   NULL_POINTER if treasures_out or count_out is NULL
 *   INTERNAL_ERROR if player is invalid
 *   NO_MEMORY on allocation failure
 *
 * Ownership:
 *   The caller owns the returned array and must free it.
 */
Status game_engine_get_player_collected_treasures(const GameEngine *eng,
                                                   const Treasure * const **treasures_out,
                                                   int *count_out);

/*
 * game_engine_enter_portal
 * ------------------------
 * Attempt to enter a portal from the player's current tile.
 *
 * Behavior:
 *   • If the player is standing on a portal tile, transition to the portal's target room.
 *   • If the portal is gated and its switch is not pressed, entry is blocked.
 *
 * Returns:
 *   OK on success
 *   INVALID_ARGUMENT if inputs are invalid
 *   ROOM_NO_PORTAL if player is not on a portal tile
 *   ROOM_IMPASSABLE if the portal is currently locked
 *   GE_NO_SUCH_ROOM if a referenced room does not exist
 *   INTERNAL_ERROR on invariant failure
 */
Status game_engine_enter_portal(GameEngine *eng);

#endif /* GAME_ENGINE_ACCESSORS_H */
