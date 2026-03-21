"""
High-level Python wrapper for the C Player type.

Provides a Pythonic interface to player operations.
"""

import ctypes
from ..bindings import lib, Status, Treasure
from .exceptions import status_to_exception


class Player:
    """Wrapper for C Player type."""

    def __init__(self, ptr: ctypes.c_void_p) -> None:
        """
        Initialize a Player wrapper.

        Args:
            ptr: Pointer to the C Player object
        """
        self._ptr = ptr

    def get_room(self) -> int:
        """
        Get the ID of the room the player is currently in.

        Returns:
            Room ID
        """
        return lib.player_get_room(self._ptr)

    def get_position(self) -> tuple[int, int]:
        """
        Get the player's current position.

        Returns:
            Tuple of (x, y) coordinates
        """
        x = ctypes.c_int()
        y = ctypes.c_int()
        status = lib.player_get_position(self._ptr, ctypes.byref(x), ctypes.byref(y))

        if status != Status.OK:
            raise status_to_exception(status, "Failed to get player position")

        return (x.value, y.value)

    def get_collected_count(self) -> int:
        """
        Get the number of treasures collected.

        Returns:
            Number of collected treasures
        """
        return lib.player_get_collected_count(self._ptr)

    def has_collected_treasure(self, treasure_id: int) -> bool:
        """
        Check if a specific treasure has been collected.

        Args:
            treasure_id: ID of the treasure to check

        Returns:
            True if collected, False otherwise
        """
        return lib.player_has_collected_treasure(self._ptr, treasure_id)

    def get_collected_treasures(self) -> list[dict]:
        """
        Get the list of collected treasures.

        Returns:
            List of dicts with keys: id, name, starting_room_id, initial_x,
            initial_y, x, y, collected
        """
        count = ctypes.c_int()
        treasures_ptr = lib.player_get_collected_treasures(self._ptr, ctypes.byref(count))

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
