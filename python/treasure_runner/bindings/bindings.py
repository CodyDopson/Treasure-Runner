"""
 Low-level ctypes bindings

This module provides direct ctypes access to the C library functions.
It handles:
  - Loading the shared library
  - Defining C enums and structures
  - Wrapping C function signatures
  - Managing error codes from the C layer

This is a thin layer - no error handling or convenience wrappers.
All error handling is done in the models layer.
"""

import ctypes
import os
from enum import IntEnum
from pathlib import Path


# ============================================================
# Enums matching C definitions
# ============================================================

class Direction(IntEnum):
    """Movement directions (matches DIR_* in types.h)."""
    NORTH = 0
    SOUTH = 1
    EAST = 2
    WEST = 3


class Status(IntEnum):
    """Status codes for room and player operations."""
    OK = 0
    INVALID_ARGUMENT = 1
    NULL_POINTER = 2
    NO_MEMORY = 3
    BOUNDS_EXCEEDED = 4
    INTERNAL_ERROR = 5
    ROOM_IMPASSABLE = 6
    ROOM_NO_PORTAL = 7
    ROOM_NOT_FOUND = 8
    GE_NO_SUCH_ROOM = 9
    WL_ERR_CONFIG = 10
    WL_ERR_DATAGEN = 11

# Backwards compatibility for existing imports
GameEngineStatus = Status


# ============================================================
# C Structures - Opaque types only
# ============================================================

# Treasure is used by player_get_collected_treasures
class Treasure(ctypes.Structure):
    _fields_ = [
        ("id", ctypes.c_int),
        ("name", ctypes.c_char_p),
        ("starting_room_id", ctypes.c_int),
        ("initial_x", ctypes.c_int),
        ("initial_y", ctypes.c_int),
        ("x", ctypes.c_int),
        ("y", ctypes.c_int),
        ("collected", ctypes.c_bool),
    ]


# ============================================================
# Library Loading
# ============================================================

def _find_library():
    """Locate libbackend.so under the project dist directory."""
    # Optional override via env
    env_path = os.getenv("TREASURE_RUNNER_DIST")
    candidates = []

    if env_path:
        candidates.append(Path(env_path) / "libbackend.so")
        candidates.append(Path(env_path) / "libpuzzlegen.so")

    # Project-relative: ../../dist relative to this file
    here = Path(__file__).resolve()
    repo_root = here.parent.parent.parent.parent
    candidates.append(repo_root / "dist" / "libbackend.so")
    candidates.append(repo_root / "dist" / "libpuzzlegen.so")

    found = {}
    for path in candidates:
        if path.exists():
            found[path.name] = path

    if "libbackend.so" in found:
        # Ensure puzzlegen is loaded first if present to satisfy dependencies.
        puzzlegen = found.get("libpuzzlegen.so")
        if puzzlegen:
            ctypes.CDLL(str(puzzlegen))
        return str(found["libbackend.so"])

    tried = "\n".join(str(p) for p in candidates)
    raise RuntimeError(f"libbackend.so not found. Paths tried:\n{tried}")


# Load the library
_LIB_PATH = _find_library()
lib = ctypes.CDLL(_LIB_PATH)


# ============================================================
# C Function Signatures
# ============================================================

# Opaque pointer type for GameEngine
GameEngine = ctypes.c_void_p

# Opaque pointer type for Player
Player = ctypes.c_void_p

# Room is opaque - no direct room accessors exposed to Python
Room = ctypes.c_void_p


# ============================================================
# Game Engine Lifecycle
# ============================================================

# game_engine_create(const char *config_file_path, GameEngine **engine_out)
lib.game_engine_create.argtypes = [
    ctypes.c_char_p,
    ctypes.POINTER(GameEngine)
]
lib.game_engine_create.restype = Status

# game_engine_destroy(GameEngine *eng)
lib.game_engine_destroy.argtypes = [GameEngine]
lib.game_engine_destroy.restype = None


# ============================================================
# Game Engine Operations
# ============================================================

# game_engine_get_player(const GameEngine *eng) -> const Player *
lib.game_engine_get_player.argtypes = [GameEngine]
lib.game_engine_get_player.restype = Player

# game_engine_move_player(GameEngine *eng, Direction dir)
lib.game_engine_move_player.argtypes = [GameEngine, ctypes.c_int]
lib.game_engine_move_player.restype = Status

# game_engine_render_current_room(const GameEngine *eng, char **str_out)
lib.game_engine_render_current_room.argtypes = [
    GameEngine,
    ctypes.POINTER(ctypes.c_char_p)
]
lib.game_engine_render_current_room.restype = Status

# game_engine_get_room_count(const GameEngine *eng, int *count_out)
lib.game_engine_get_room_count.argtypes = [
    GameEngine,
    ctypes.POINTER(ctypes.c_int)
]
lib.game_engine_get_room_count.restype = Status

# game_engine_get_room_dimensions(const GameEngine *eng, int *width_out, int *height_out)
lib.game_engine_get_room_dimensions.argtypes = [
    GameEngine,
    ctypes.POINTER(ctypes.c_int),
    ctypes.POINTER(ctypes.c_int)
]
lib.game_engine_get_room_dimensions.restype = Status

# game_engine_get_room_ids(const GameEngine *eng, int **ids_out, int *count_out)
lib.game_engine_get_room_ids.argtypes = [
    GameEngine,
    ctypes.POINTER(ctypes.POINTER(ctypes.c_int)),
    ctypes.POINTER(ctypes.c_int)
]
lib.game_engine_get_room_ids.restype = Status

# game_engine_reset(GameEngine *eng)
lib.game_engine_reset.argtypes = [GameEngine]
lib.game_engine_reset.restype = Status

# game_engine_get_player_room(const GameEngine *eng, int *room_out)
lib.game_engine_get_player_room.argtypes = [
    GameEngine,
    ctypes.POINTER(ctypes.c_int)
]
lib.game_engine_get_player_room.restype = Status

# game_engine_get_player_position(const GameEngine *eng, int *x_out, int *y_out)
lib.game_engine_get_player_position.argtypes = [
    GameEngine,
    ctypes.POINTER(ctypes.c_int),
    ctypes.POINTER(ctypes.c_int)
]
lib.game_engine_get_player_position.restype = Status

# game_engine_get_player_collected_count(const GameEngine *eng, int *count_out)
lib.game_engine_get_player_collected_count.argtypes = [
    GameEngine,
    ctypes.POINTER(ctypes.c_int)
]
lib.game_engine_get_player_collected_count.restype = Status

# game_engine_get_total_treasure_count(const GameEngine *eng, int *count_out)
lib.game_engine_get_total_treasure_count.argtypes = [
    GameEngine,
    ctypes.POINTER(ctypes.c_int)
]
lib.game_engine_get_total_treasure_count.restype = Status

# game_engine_is_game_over(const GameEngine *eng, bool *is_over_out)
lib.game_engine_is_game_over.argtypes = [
    GameEngine,
    ctypes.POINTER(ctypes.c_bool)
]
lib.game_engine_is_game_over.restype = Status

# game_engine_is_victory(const GameEngine *eng, bool *is_victory_out)
lib.game_engine_is_victory.argtypes = [
    GameEngine,
    ctypes.POINTER(ctypes.c_bool)
]
lib.game_engine_is_victory.restype = Status

# game_engine_player_has_collected_treasure(const GameEngine *eng, int treasure_id, bool *has_out)
lib.game_engine_player_has_collected_treasure.argtypes = [
    GameEngine,
    ctypes.c_int,
    ctypes.POINTER(ctypes.c_bool)
]
lib.game_engine_player_has_collected_treasure.restype = Status

# game_engine_get_player_collected_treasures(const GameEngine *eng, const Treasure * const **treasures_out, int *count_out)
lib.game_engine_get_player_collected_treasures.argtypes = [
    GameEngine,
    ctypes.POINTER(ctypes.POINTER(ctypes.POINTER(Treasure))),
    ctypes.POINTER(ctypes.c_int)
]
lib.game_engine_get_player_collected_treasures.restype = Status


# ============================================================
# Memory Management
# ============================================================

# game_engine_free_string(void *ptr)
lib.game_engine_free_string.argtypes = [ctypes.c_void_p]
lib.game_engine_free_string.restype = None

# destroy_treasure(Treasure *t)
lib.destroy_treasure.argtypes = [ctypes.POINTER(Treasure)]
lib.destroy_treasure.restype = None
