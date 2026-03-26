"""Game UI module - handles all curses-based rendering and player interactions."""

import curses
import os
from pathlib import Path

from ..bindings import Direction
from ..models.exceptions import GameEngineError
from ..models.game_engine import GameEngine
from ..models.player_profile import (
    load_or_create_profile,
    save_profile,
    update_profile_after_run,
)


class TerminalTooSmallError(RuntimeError):
    """Raised when the terminal cannot fit a full game frame."""


# Taken from lab 5, with some modifications for our game
class GameUI:
    """
    Handles all curses-based rendering and player-facing messages.

    Responsibilities:
      • Initialize curses environment and color scheme
      • Render the game board and player state
      • Display transient messages and status information
    """

    # Color pair constants (for readability and consistency)
    COLOR_DEFAULT = 0
    COLOR_ITEM = 3
    BOARD_TOP = 3
    FOOTER_LINES = 4
    MIN_WIDTH = 40
    MIN_HEIGHT = 12

    def __init__(self, stdscr) -> None:
        self._stdscr = stdscr
        self._last_message = "Welcome to Treasure Runner."

    # ---------- Initialization ----------

    def init_screen(self) -> None:
        """Initialize curses screen and visual style."""
        curses.cbreak()
        curses.noecho()
        self._stdscr.keypad(True)
        curses.start_color()
        curses.use_default_colors()
        try:
            curses.curs_set(0)
        except curses.error:
            pass
        self._init_colors()
        self._stdscr.bkgd(' ', curses.color_pair(self.COLOR_DEFAULT))
        self._clear_screen()

    @classmethod
    def _init_colors(cls) -> None:
        """Define color pairs for normal tiles and special cases."""
        try:
            curses.init_pair(cls.COLOR_DEFAULT, curses.COLOR_BLACK, 15)
            curses.init_pair(cls.COLOR_ITEM, curses.COLOR_GREEN, -1)
        except curses.error:
            pass

    def _clear_screen(self) -> None:
        """Clear and refresh the screen."""
        self._stdscr.clear()
        self._stdscr.refresh()

    # ---------- Rendering ----------

    def render(self, engine: "GameEngine", profile_path: str = "", stats: dict | None = None) -> None:
        """Redraw room and status by querying current GameEngine state."""
        room_text = engine.render_current_room()
        self._clear_screen()

        max_y, max_x = self._stdscr.getmaxyx()
        lines = room_text.splitlines()
        board_width = max((len(line) for line in lines), default=0)
        board_height = len(lines)

        required_width = min(max(board_width + 16, self.MIN_WIDTH), 5000)
        required_height = max(self.BOARD_TOP + board_height + self.FOOTER_LINES + 1, self.MIN_HEIGHT)
        if max_x < required_width or max_y < required_height:
            raise TerminalTooSmallError(
                f"Terminal too small: need at least {required_width}x{required_height}, got {max_x}x{max_y}."
            )

        self._draw_message_bar(max_x)
        self._draw_room_header(engine.get_player_room(), max_x)

        self._draw_board(lines, max_x, max_y)

        self._draw_legend(max_x, board_width)
        self._draw_status(engine, profile_path, stats)
        self._draw_title_and_email(max_x)
        self._stdscr.refresh()

    def _draw_status(self, engine: "GameEngine", profile_path: str = "", stats: dict | None = None) -> None:
        """Show player progress and controls above the game footer."""
        max_y, max_x = self._stdscr.getmaxyx()
        self._stdscr.move(max_y - 3, 0)
        self._stdscr.clrtoeol()
        status_text = self._status_text(engine, profile_path, stats)
        self._safe_addstr(max_y - 3, 0, status_text, curses.A_BOLD)

        self._stdscr.move(max_y - 2, 0)
        self._stdscr.clrtoeol()
        self._safe_addstr(max_y - 2, 0, "Controls: Arrows/WASD move | r reset | q quit")

    def _draw_board(self, lines: list[str], max_x: int, max_y: int) -> None:
        board_bottom = max_y - self.FOOTER_LINES - 1
        for y, line in enumerate(lines, start=self.BOARD_TOP):
            if y > board_bottom:
                break
            for x, tile in enumerate(line):
                if x >= max_x - 1:
                    break
                self._safe_addch(y, x, tile, self._color_for_tile(tile))

    def _status_text(self, engine: "GameEngine", profile_path: str, stats: dict | None) -> str:
        details = stats or {}
        rooms_played = details.get("rooms_played", 1)
        rooms_left = max(details.get("total_rooms", 0) - rooms_played, 0)
        total_treasures = int(details.get("total_treasures", engine.get_total_treasure_count()))
        collected_treasures = engine.get_player_collected_count()
        profile_name = os.path.basename(profile_path) if profile_path else "-"
        position = engine.get_player_position()
        return (
            f"Status: Treasures={collected_treasures}/{total_treasures} Rooms Played={rooms_played} "
            f"Rooms Left={rooms_left} Room={engine.get_player_room()} Pos=({position[0]},{position[1]}) "
            f"Profile={profile_name}"
        )

    # ---------- User Input ----------

    def read_key(self) -> int:
        """Read a single key from the terminal window."""
        return self._stdscr.getch()

    @staticmethod
    def is_quit_key(key: int) -> bool:
        """Return True when key matches quit commands."""
        return key in (ord('q'), ord('Q'))

    @staticmethod
    def is_reset_key(key: int) -> bool:
        """Return True when key matches reset commands."""
        return key in (ord('r'), ord('R'))

    @staticmethod
    def is_portal_key(key: int) -> bool:
        """Return True when key matches portal interaction commands."""
        return key == ord('>')

    def read_direction(self, key: int):
        """Translate a curses key into a Direction or None."""
        mapping = {
            ord('w'): Direction.NORTH,
            ord('W'): Direction.NORTH,
            ord('s'): Direction.SOUTH,
            ord('S'): Direction.SOUTH,
            ord('a'): Direction.WEST,
            ord('A'): Direction.WEST,
            ord('d'): Direction.EAST,
            ord('D'): Direction.EAST,
            curses.KEY_UP: Direction.NORTH,
            curses.KEY_DOWN: Direction.SOUTH,
            curses.KEY_LEFT: Direction.WEST,
            curses.KEY_RIGHT: Direction.EAST,
        }

        return mapping.get(key)

    # ---------- Messaging ----------

    def message(self, msg: str) -> None:
        """Display a transient message at the top of the screen."""
        self._last_message = msg
        _, max_x = self._stdscr.getmaxyx()
        self._draw_message_bar(max_x)
        self._stdscr.refresh()

    def _draw_message_bar(self, max_x: int) -> None:
        self._stdscr.move(0, 0)
        self._stdscr.clrtoeol()
        if self._last_message:
            text = f"Message: {self._last_message}"
            self._safe_addstr(0, 0, text, curses.A_BOLD)

    def _draw_room_header(self, room_id: int, max_x: int) -> None:
        self._stdscr.move(1, 0)
        self._stdscr.clrtoeol()
        header = f"Room {room_id}"
        self._safe_addstr(1, 0, header, curses.A_UNDERLINE)

    def _draw_legend(self, max_x: int, board_width: int) -> None:
        legend_x = min(board_width + 2, max_x - 1)
        legend_lines = [
            "Elements:",
            "@ player",
            "# wall",
            "$ gold",
            "x exit",
        ]
        for index, text in enumerate(legend_lines):
            y = self.BOARD_TOP + index
            if legend_x < max_x - 1:
                self._stdscr.move(y, legend_x)
                self._stdscr.clrtoeol()
                self._safe_addstr(y, legend_x, text)

    def _draw_title_and_email(self, max_x: int) -> None:
        max_y, _ = self._stdscr.getmaxyx()
        title_y = max_y - 1
        self._stdscr.move(title_y, 0)
        self._stdscr.clrtoeol()
        title = "Treasure Runner | contact: cdopson@uoguelph.ca"
        self._safe_addstr(title_y, 0, title, curses.A_DIM)

    def prompt_player_name(self) -> str:
        """Prompt user for a player name for new profile creation."""
        self._clear_screen()
        max_y, max_x = self._stdscr.getmaxyx()
        prompt = "Enter player name: "
        header = "No profile found. Creating a new player profile."
        self._safe_addstr(max(max_y // 2 - 1, 0), 0, header, curses.A_BOLD)
        self._safe_addstr(max(max_y // 2, 0), 0, prompt)
        self._stdscr.refresh()

        curses.echo()
        try:
            raw = self._stdscr.getstr(max(max_y // 2, 0), min(len(prompt), max_x - 1), 48)
        finally:
            curses.noecho()

        return raw.decode("utf-8", errors="ignore").strip() or "Player"

    def show_profile_summary(self, title: str, profile: dict, footer: str) -> None:
        """Render profile summary screen using curses and wait for keypress."""
        self._clear_screen()
        max_y, max_x = self._stdscr.getmaxyx()
        lines = [
            title,
            "",
            f"Player: {profile.get('player_name', 'Player')}",
            f"Games Played: {profile.get('games_played', 0)}",
            f"Max Treasure Collected: {profile.get('max_treasure_collected', 0)}",
            f"Most Rooms Completed: {profile.get('most_rooms_world_completed', 0)}",
            f"Last Played: {profile.get('timestamp_last_played', '-')}",
            "",
            footer,
        ]

        start_y = max((max_y - len(lines)) // 2, 0)
        for offset, line in enumerate(lines):
            y = start_y + offset
            if y >= max_y:
                break
            self._safe_addstr(y, 0, line, curses.A_BOLD if offset == 0 else 0)

        self._stdscr.refresh()
        self._stdscr.getch()

    # ---------- Internal helpers ----------

    def _color_for_tile(self, tile: str) -> int:
        """
        Return the curses attribute for this tile.

        Used by both render() and draw_tile() to ensure consistent coloring.
        """
        if tile.isalpha() and tile != '@':
            return curses.color_pair(self.COLOR_ITEM)
        if tile in ('|', '-'):
            return curses.color_pair(self.COLOR_DEFAULT) | curses.A_DIM

        return curses.color_pair(self.COLOR_DEFAULT)

    def _safe_addstr(self, y: int, x: int, text: str, attr: int = 0) -> None:
        """Best-effort draw that never raises curses.error for tight layouts."""
        max_y, max_x = self._stdscr.getmaxyx()
        if y < 0 or y >= max_y or x < 0 or x >= max_x:
            return

        width = max_x - x - 1
        if width <= 0:
            return

        try:
            self._stdscr.addstr(y, x, text[:width], attr)
        except curses.error:
            pass

    def _safe_addch(self, y: int, x: int, tile: str, attr: int = 0) -> None:
        """Best-effort character draw that avoids curses boundary crashes."""
        max_y, max_x = self._stdscr.getmaxyx()
        if y < 0 or y >= max_y or x < 0 or x >= max_x:
            return

        try:
            self._stdscr.addch(y, x, tile, attr)
        except curses.error:
            pass


def _curses_main(stdscr, config_path: str, profile_path: str) -> int:
    """Initialize the view and execute the controller loop."""
    ui_view = GameUI(stdscr)
    ui_view.init_screen()
    profile, _ = load_or_create_profile(profile_path, ui_view.prompt_player_name)
    ui_view.show_profile_summary("Player Profile", profile, "Press any key to start game")

    engine = None
    exit_code = 1
    try:
        engine = GameEngine(config_path)
        exit_code = engine.run(ui_view, profile_path)
        return exit_code
    except TerminalTooSmallError as exc:
        ui_view.message(str(exc))
        ui_view.read_key()
        return 1
    except GameEngineError as exc:
        ui_view.message(f"Error: {exc}")
        return 1
    finally:
        run_treasure_collected = 0
        run_rooms_completed = 0
        if engine is not None:
            try:
                stats = engine.get_last_run_stats()
                run_treasure_collected = int(stats.get("treasure_collected", 0))
                run_rooms_completed = int(stats.get("rooms_completed", 0))
            except GameEngineError:
                run_treasure_collected = 0
                run_rooms_completed = 0
            engine.destroy()

        updated_profile = update_profile_after_run(
            profile,
            run_treasure_collected,
            run_rooms_completed,
        )
        save_profile(profile_path, updated_profile)
        ui_view.show_profile_summary("Session Summary", updated_profile, "Press any key to exit")


def _resolve_profile_path(profile_path: str) -> str:
    """Force profile storage under assets/ while preserving filename from user input."""
    project_root = Path(__file__).resolve().parents[3]
    assets_dir = project_root / "assets"
    assets_dir.mkdir(parents=True, exist_ok=True)
    name = Path(profile_path).name or "player_profile.json"
    return str((assets_dir / name).resolve())


def run_game(config_path: str, profile_path: str) -> int:
    """Public entry point used by python/run_game.py."""
    try:
        normalized_profile_path = _resolve_profile_path(profile_path)
        return int(curses.wrapper(_curses_main, config_path, normalized_profile_path) or 0)
    except GameEngineError as exc:
        print(f"Error: {exc}")
        return 1


def main(config_path: str, profile_path: str) -> int:
    """Alias entry point for launchers that expect main()."""
    return run_game(config_path, profile_path)


def launch(config_path: str, profile_path: str) -> int:
    """Alias entry point for launchers that expect launch()."""
    return run_game(config_path, profile_path)
