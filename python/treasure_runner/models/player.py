"""
High-level Python wrapper for the C Player type.

Provides a Pythonic interface to player operations.
"""

import ctypes
from collections.abc import Callable
from ..bindings import lib, Status, Treasure
from .exceptions import status_to_exception, GameEngineError


class Player:
    """Wrapper for C Player type."""

    def __init__(self, engine_ptr_getter: Callable[[], ctypes.c_void_p]) -> None:
        """
        Initialize a Player wrapper.

        Args:
            engine_ptr_getter: Callable returning the owning C GameEngine pointer
        """
        self._engine_ptr_getter = engine_ptr_getter

    def _engine_ptr(self) -> ctypes.c_void_p:
        """Return a live GameEngine pointer or raise if engine is destroyed."""
        ptr = self._engine_ptr_getter()
        if not ptr:
            raise GameEngineError("Game engine is not available")
        return ptr

    def get_room(self) -> int:
        """
        Get the ID of the room the player is currently in.

        Returns:
            Room ID
        """
        room_id = ctypes.c_int()
        status = lib.game_engine_get_player_room(self._engine_ptr(), ctypes.byref(room_id))
        if status != Status.OK:
            raise status_to_exception(status, "Failed to get player room")
        return room_id.value

    def get_position(self) -> tuple[int, int]:
        """
        Get the player's current position.

        Returns:
            Tuple of (x, y) coordinates
        """
        x = ctypes.c_int()
        y = ctypes.c_int()
        status = lib.game_engine_get_player_position(self._engine_ptr(), ctypes.byref(x), ctypes.byref(y))

        if status != Status.OK:
            raise status_to_exception(status, "Failed to get player position")

        return (x.value, y.value)

    def get_collected_count(self) -> int:
        """
        Get the number of treasures collected.

        Returns:
            Number of collected treasures
        """
        count = ctypes.c_int()
        status = lib.game_engine_get_player_collected_count(self._engine_ptr(), ctypes.byref(count))
        if status != Status.OK:
            raise status_to_exception(status, "Failed to get collected treasure count")
        return count.value

    def has_collected_treasure(self, treasure_id: int) -> bool:
        """
        Check if a specific treasure has been collected.

        Args:
            treasure_id: ID of the treasure to check

        Returns:
            True if collected, False otherwise
        """
        collected = ctypes.c_bool()
        status = lib.game_engine_player_has_collected_treasure(
            self._engine_ptr(),
            treasure_id,
            ctypes.byref(collected),
        )
        if status != Status.OK:
            raise status_to_exception(status, f"Failed to check collected treasure {treasure_id}")
        return bool(collected.value)

    def get_collected_treasures(self) -> list[dict]:
        """
        Get the list of collected treasures.

        Returns:
            List of dicts with keys: id, name, starting_room_id, initial_x,
            initial_y, x, y, collected
        """
        count = ctypes.c_int()
        treasures_ptr = ctypes.POINTER(ctypes.POINTER(Treasure))()
        status = lib.game_engine_get_player_collected_treasures(
            self._engine_ptr(),
            ctypes.byref(treasures_ptr),
            ctypes.byref(count),
        )
        if status != Status.OK:
            raise status_to_exception(status, "Failed to get collected treasures")

        result = []
        if treasures_ptr:
            for i in range(count.value):
                treasure = treasures_ptr[i].contents
                result.append({
                    "id": treasure.id,
                    "name": treasure.name.decode('utf-8') if treasure.name else None,
                    "starting_room_id": treasure.starting_room_id,
                    "initial_x": treasure.initial_x,
                    "initial_y": treasure.initial_y,
                    "x": treasure.x,
                    "y": treasure.y,
                    "collected": treasure.collected,
                })

        return result
