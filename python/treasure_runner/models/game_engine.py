"""
High-level Python wrapper for the C GameEngine type.

Provides a Pythonic interface to game operations.
"""

import ctypes
from typing import TYPE_CHECKING
from ..bindings import lib, Status, Direction, Treasure
from .exceptions import status_to_exception, GameEngineError, ImpassableError, NoPortalError
from .player import Player


if TYPE_CHECKING:
    from ..ui.game_ui import GameUI


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

        self._player = Player(lambda: self._eng)
        self._last_run_stats = {
            "rooms_completed": 0,
            "treasure_collected": 0,
            "total_treasures": 0,
            "steps_taken": 0,
            "world_completed": False,
        }

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

    def enter_portal(self) -> None:
        """Enter a portal from the current tile if one is present and unlocked."""
        status = lib.game_engine_enter_portal(self._eng)
        if status != Status.OK:
            raise status_to_exception(status, "Failed to enter portal")

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

    def run(self, ui_view: "GameUI", profile_path: str = "") -> int:
        """
        Run the main game loop by coordinating model state and the UI.

        The engine remains free of curses details and only consumes UI methods.

        Args:
            ui_view: View object responsible for rendering and collecting input
            profile_path: Player profile path supplied by launcher

        Returns:
            Process-style status code
        """
        total_rooms, total_treasures, visited_rooms = self._init_run_state()
        ui_view.message("Use WASD/arrows to move, r to reset, q to quit.")

        while True:
            if not self._render_loop_frame(ui_view, profile_path, total_rooms, total_treasures, visited_rooms):
                return 1

            if self.is_victory():
                return self._render_victory_frame(
                    ui_view,
                    profile_path,
                    total_rooms,
                    total_treasures,
                    visited_rooms,
                )

            key = ui_view.read_key()
            result = self._handle_loop_input(ui_view, key, visited_rooms, total_treasures)
            if result is not None:
                return result

    def _init_run_state(self) -> tuple[int, int, set[int]]:
        """Initialize run counters and return shared loop state."""
        total_rooms = self.get_room_count()
        total_treasures = self.get_total_treasure_count()
        visited_rooms = {self.get_player_room()}
        self._last_run_stats = {
            "rooms_completed": len(visited_rooms),
            "treasure_collected": self.get_player_collected_count(),
            "total_treasures": total_treasures,
            "steps_taken": 0,
            "world_completed": False,
        }
        return total_rooms, total_treasures, visited_rooms

    def _render_victory_frame(
        self,
        ui_view: "GameUI",
        profile_path: str,
        total_rooms: int,
        total_treasures: int,
        visited_rooms: set[int],
    ) -> int:
        """Render final victory message and frame, then terminate successfully."""
        rooms_visited = len(visited_rooms)
        profile_name = profile_path.rsplit("/", 1)[-1] if profile_path else "-"
        steps_taken = int(self._last_run_stats.get("steps_taken", 0))
        collected_count = self.get_player_collected_count()

        # Finalize run stats at the exact victory point so profile persistence sees
        # the latest room and treasure totals even when exiting immediately.
        self._last_run_stats["rooms_completed"] = max(
            int(self._last_run_stats.get("rooms_completed", 0)),
            rooms_visited,
        )
        self._last_run_stats["treasure_collected"] = collected_count
        self._last_run_stats["total_treasures"] = total_treasures
        self._last_run_stats["world_completed"] = True

        ui_view.message(
            "Victory! "
            f"Profile={profile_name} | "
            f"Treasures={collected_count}/{total_treasures} | "
            f"Steps={steps_taken} | Rooms Visited={rooms_visited}"
        )
        ui_view.render(
            self,
            profile_path,
            {
                "total_rooms": total_rooms,
                "rooms_played": rooms_visited,
                "total_treasures": total_treasures,
            },
        )
        return 0

    def _render_loop_frame(
        self,
        ui_view: "GameUI",
        profile_path: str,
        total_rooms: int,
        total_treasures: int,
        visited_rooms: set[int],
    ) -> bool:
        try:
            visited_rooms.add(self.get_player_room())
            self._last_run_stats["rooms_completed"] = len(visited_rooms)
            self._last_run_stats["treasure_collected"] = self.get_player_collected_count()
            self._last_run_stats["total_treasures"] = total_treasures
            ui_view.render(
                self,
                profile_path,
                {
                    "total_rooms": total_rooms,
                    "rooms_played": len(visited_rooms),
                    "total_treasures": total_treasures,
                },
            )
        except GameEngineError as exc:
            ui_view.message(f"Render failed: {exc}")
            return False

        return True

    def get_last_run_stats(self) -> dict:
        """Return run statistics captured during the most recent loop."""
        return {
            "rooms_completed": int(self._last_run_stats.get("rooms_completed", 0)),
            "treasure_collected": int(self._last_run_stats.get("treasure_collected", 0)),
            "total_treasures": int(self._last_run_stats.get("total_treasures", 0)),
            "steps_taken": int(self._last_run_stats.get("steps_taken", 0)),
            "world_completed": bool(self._last_run_stats.get("world_completed", False)),
        }

    def _handle_loop_input(
        self,
        ui_view: "GameUI",
        key: int,
        visited_rooms: set[int],
        total_treasures: int,
    ) -> int | None:
        if ui_view.is_quit_key(key):
            return 0

        if ui_view.is_reset_key(key):
            self._handle_reset(ui_view, visited_rooms)
            return None

        if ui_view.is_portal_key(key):
            self._handle_portal_enter(ui_view)
            return None

        direction = ui_view.read_direction(key)
        if direction is None:
            return None

        self._handle_move(ui_view, direction, total_treasures)
        return None

    def _handle_reset(self, ui_view: "GameUI", visited_rooms: set[int]) -> None:
        try:
            self.reset()
            visited_rooms.clear()
            visited_rooms.add(self.get_player_room())
            self._last_run_stats["steps_taken"] = 0
            self._last_run_stats["world_completed"] = False
            ui_view.message("Game reset to initial state.")
        except GameEngineError as exc:
            ui_view.message(f"Reset failed: {exc}")

    def _handle_move(self, ui_view: "GameUI", direction: Direction, total_treasures: int) -> None:
        try:
            previous_collected = self.get_player_collected_count()
            self.move_player(direction)
            self._last_run_stats["steps_taken"] = int(self._last_run_stats.get("steps_taken", 0)) + 1
            current_collected = self.get_player_collected_count()
            if current_collected > previous_collected:
                if 0 < total_treasures <= current_collected:
                    ui_view.message(f"Treasure progress: {current_collected}/{total_treasures} treasures collected")
                else:
                    ui_view.message(f"You picked up a treasure ({current_collected}/{total_treasures})")
            else:
                ui_view.message(f"Treasure progress: {current_collected}/{total_treasures} treasures collected")
        except ImpassableError:
            ui_view.message("That way is blocked.")
        except GameEngineError as exc:
            ui_view.message(f"Move failed: {exc}")

    def _handle_portal_enter(self, ui_view: "GameUI") -> None:
        """Try entering the portal under the player, if any."""
        try:
            self.enter_portal()
            ui_view.message("You enter the portal.")
        except NoPortalError:
            ui_view.message("Stand on a portal tile before using >.")
        except ImpassableError:
            ui_view.message("Portal is locked.")
        except GameEngineError as exc:
            ui_view.message(f"Portal entry failed: {exc}")

    def get_player_room(self) -> int:
        """Return the current room ID via the game engine API."""
        room_id = ctypes.c_int()
        status = lib.game_engine_get_player_room(self._eng, ctypes.byref(room_id))
        if status != Status.OK:
            raise status_to_exception(status, "Failed to get player room")
        return room_id.value

    def get_player_position(self) -> tuple[int, int]:
        """Return the player's (x, y) position via the game engine API."""
        x = ctypes.c_int()
        y = ctypes.c_int()
        status = lib.game_engine_get_player_position(self._eng, ctypes.byref(x), ctypes.byref(y))
        if status != Status.OK:
            raise status_to_exception(status, "Failed to get player position")
        return (x.value, y.value)

    def get_player_collected_count(self) -> int:
        """Return number of collected treasures via the game engine API."""
        count = ctypes.c_int()
        status = lib.game_engine_get_player_collected_count(self._eng, ctypes.byref(count))
        if status != Status.OK:
            raise status_to_exception(status, "Failed to get collected treasure count")
        return count.value

    def get_total_treasure_count(self) -> int:
        """Return total number of world treasures via the game engine API."""
        count = ctypes.c_int()
        status = lib.game_engine_get_total_treasure_count(self._eng, ctypes.byref(count))
        if status != Status.OK:
            raise status_to_exception(status, "Failed to get total treasure count")
        return count.value

    def is_game_over(self) -> bool:
        """Return whether the game has ended in a terminal state."""
        value = ctypes.c_bool()
        status = lib.game_engine_is_game_over(self._eng, ctypes.byref(value))
        if status != Status.OK:
            raise status_to_exception(status, "Failed to get game-over state")
        return bool(value.value)

    def is_victory(self) -> bool:
        """Return whether all treasures have been collected."""
        value = ctypes.c_bool()
        status = lib.game_engine_is_victory(self._eng, ctypes.byref(value))
        if status != Status.OK:
            raise status_to_exception(status, "Failed to get victory state")
        return bool(value.value)

    def player_has_collected_treasure(self, treasure_id: int) -> bool:
        """Check whether a treasure ID has been collected via the game engine API."""
        collected = ctypes.c_bool()
        status = lib.game_engine_player_has_collected_treasure(
            self._eng,
            treasure_id,
            ctypes.byref(collected),
        )
        if status != Status.OK:
            raise status_to_exception(status, f"Failed to check collected treasure {treasure_id}")
        return bool(collected.value)

    def get_player_collected_treasures(self) -> list[dict]:
        """Return collected treasures as Python dicts via the game engine API."""
        count = ctypes.c_int()
        treasures_ptr = ctypes.POINTER(ctypes.POINTER(Treasure))()

        status = lib.game_engine_get_player_collected_treasures(
            self._eng,
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
