"""
High-level Python wrapper for the C GameEngine type.

Provides a Pythonic interface to game operations.
"""

import ctypes
from typing import TYPE_CHECKING
from ..bindings import lib, Status, Direction
from .exceptions import status_to_exception, GameEngineError, ImpassableError
from .player import Player


if TYPE_CHECKING:
    from .game_ui import GameUI


class GameEngine:
    """Wrapper for C GameEngine type."""

    def __init__(self, config_path: str) -> None:
        """
        Initialize a GameEngine.

        Creates a new game engine by loading the world configuration.

        Args:
            config_path: Path to the world configuration file

        Raises:
            GameEngineError or subclass if creation fails
            RuntimeError if player initialization fails
        """
        self._eng = ctypes.c_void_p()

        # Convert config_path to bytes for C string
        config_bytes = config_path.encode('utf-8')

        # Create the engine
        status = lib.game_engine_create(config_bytes, ctypes.byref(self._eng))
        if status != Status.OK:
            raise status_to_exception(status, f"Failed to create game engine with config: {config_path}")

        # Get the player
        player_ptr = lib.game_engine_get_player(self._eng)
        if not player_ptr:
            raise RuntimeError("Failed to initialize player")

        self._player = Player(player_ptr)

    @property
    def player(self) -> Player:
        """
        Get the current player.

        Returns:
            Player object
        """
        return self._player

    def destroy(self) -> None:
        """
        Destroy the game engine and free all resources.

        Safe to call multiple times.
        """
        if self._eng:
            lib.game_engine_destroy(self._eng)
            self._eng = None

    def move_player(self, direction: Direction) -> None:
        """
        Move the player in the specified direction.

        Args:
            direction: Direction to move (Direction enum value)

        Raises:
            GameEngineError or subclass if movement fails
        """
        status = lib.game_engine_move_player(self._eng, direction)
        if status != Status.OK:
            raise status_to_exception(status, f"Failed to move player in direction {direction}")

    def render_current_room(self) -> str:
        """
        Render the player's current room as a string.

        Returns:
            Multi-line string representation of the room

        Raises:
            GameEngineError or subclass if rendering fails
        """
        str_out = ctypes.c_char_p()
        status = lib.game_engine_render_current_room(self._eng, ctypes.byref(str_out))

        if status != Status.OK:
            raise status_to_exception(status, "Failed to render current room")

        # Decode the C string
        result = (str_out.value and str_out.value.decode('utf-8')) or ""
        # Free the C-allocated string
        lib.game_engine_free_string(str_out)

        return result

    def get_room_count(self) -> int:
        """
        Get the total number of rooms in the world.

        Returns:
            Number of rooms

        Raises:
            GameEngineError or subclass if query fails
        """
        count = ctypes.c_int()
        status = lib.game_engine_get_room_count(self._eng, ctypes.byref(count))

        if status != Status.OK:
            raise status_to_exception(status, "Failed to get room count")

        return count.value

    def get_room_dimensions(self) -> tuple[int, int]:
        """
        Get the width and height of the player's current room.

        Returns:
            Tuple of (width, height)

        Raises:
            GameEngineError or subclass if query fails
        """
        width = ctypes.c_int()
        height = ctypes.c_int()
        status = lib.game_engine_get_room_dimensions(self._eng, ctypes.byref(width), ctypes.byref(height))

        if status != Status.OK:
            raise status_to_exception(status, "Failed to get room dimensions")

        return (width.value, height.value)

    def get_room_ids(self) -> list[int]:
        """
        Get the IDs of all rooms in the world.

        Returns:
            List of room IDs

        Raises:
            GameEngineError or subclass if query fails
        """
        ids_out = ctypes.POINTER(ctypes.c_int)()
        count = ctypes.c_int()
        status = lib.game_engine_get_room_ids(self._eng, ctypes.byref(ids_out), ctypes.byref(count))

        if status != Status.OK:
            raise status_to_exception(status, "Failed to get room IDs")

        # Copy IDs from C array to Python list
        result = []
        if ids_out:
            for i in range(count.value):
                result.append(ids_out[i])

            # Free the C-allocated array
            lib.game_engine_free_string(ctypes.cast(ids_out, ctypes.c_void_p))

        return result

    def reset(self) -> None:
        """
        Reset the game to its initial state.

        Resets player position, treasures, and pushables.

        Raises:
            GameEngineError or subclass if reset fails
        """
        status = lib.game_engine_reset(self._eng)
        if status != Status.OK:
            raise status_to_exception(status, "Failed to reset game")

    def run(self, ui: "GameUI", profile_path: str = "") -> int:
        """
        Run the main game loop by coordinating model state and the UI.

        The engine remains free of curses details and only consumes UI methods.

        Args:
            ui: View object responsible for rendering and collecting input
            profile_path: Player profile path supplied by launcher

        Returns:
            Process-style status code
        """
        ui.message("Use WASD/arrows to move, q to quit.")

        while True:
            ui.render(self.render_current_room(), self.player, profile_path)
            key = ui.read_key()

            if ui.is_quit_key(key):
                return 0

            direction = ui.read_direction(key)
            if direction is None:
                continue

            try:
                self.move_player(direction)
            except ImpassableError:
                ui.message("That way is blocked.")
            except GameEngineError as exc:
                ui.message(f"Move failed: {exc}")
