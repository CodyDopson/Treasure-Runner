import curses
import os

from ..bindings import Direction
from .exceptions import GameEngineError
from .game_engine import GameEngine


#Taken from lab 5, with some modifcations for our game
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
        self._stdscr.addstr(max_y - 3, 0, status_text[:max_x - 1], curses.A_BOLD)

        self._stdscr.move(max_y - 2, 0)
        self._stdscr.clrtoeol()
        self._stdscr.addstr(max_y - 2, 0, "Controls: Arrows/WASD move | r reset | q quit"[:max_x - 1])

    def _draw_board(self, lines: list[str], max_x: int, max_y: int) -> None:
        board_bottom = max_y - self.FOOTER_LINES - 1
        for y, line in enumerate(lines, start=self.BOARD_TOP):
            if y > board_bottom:
                break
            for x, tile in enumerate(line):
                if x >= max_x - 1:
                    break
                self._stdscr.addch(y, x, tile, self._color_for_tile(tile))

    def _status_text(self, engine: "GameEngine", profile_path: str, stats: dict | None) -> str:
        details = stats or {}
        rooms_played = details.get("rooms_played", 1)
        rooms_left = max(details.get("total_rooms", 0) - rooms_played, 0)
        profile_name = os.path.basename(profile_path) if profile_path else "-"
        position = engine.get_player_position()
        return (
            f"Status: Gold={engine.get_player_collected_count()} Rooms Played={rooms_played} "
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
            self._stdscr.addstr(0, 0, text[:max_x - 1], curses.A_BOLD)

    def _draw_room_header(self, room_id: int, max_x: int) -> None:
        self._stdscr.move(1, 0)
        self._stdscr.clrtoeol()
        header = f"Room {room_id}"
        self._stdscr.addstr(1, 0, header[:max_x - 1], curses.A_UNDERLINE)

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
                self._stdscr.addstr(y, legend_x, text[: max_x - legend_x - 1])

    def _draw_title_and_email(self, max_x: int) -> None:
        max_y, _ = self._stdscr.getmaxyx()
        title_y = max_y - 1
        self._stdscr.move(title_y, 0)
        self._stdscr.clrtoeol()
        title = "Treasure Runner | contact: treasure.runner@example.com"
        self._stdscr.addstr(title_y, 0, title[:max_x - 1], curses.A_DIM)

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


def _curses_main(stdscr, config_path: str, profile_path: str) -> int:
    """Initialize the view and execute the controller loop."""
    ui_view = GameUI(stdscr)
    ui_view.init_screen()

    engine = GameEngine(config_path)
    try:
        return engine.run(ui_view, profile_path)
    finally:
        engine.destroy()


def run_game(config_path: str, profile_path: str) -> int:
    """Public entry point used by python/run_game.py."""
    try:
        return int(curses.wrapper(_curses_main, config_path, profile_path) or 0)
    except GameEngineError as exc:
        print(f"Error: {exc}")
        return 1


def main(config_path: str, profile_path: str) -> int:
    """Alias entry point for launchers that expect main()."""
    return run_game(config_path, profile_path)


def launch(config_path: str, profile_path: str) -> int:
    """Alias entry point for launchers that expect launch()."""
    return run_game(config_path, profile_path)


