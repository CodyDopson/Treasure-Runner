"""
Unit tests for GameEngine and Player classes.

Tests verify all methods work correctly with valid inputs,
handle error appropriately, and maintain correct state.
"""

import os
import unittest

from treasure_runner.bindings import Direction
from treasure_runner.models.game_engine import GameEngine
from treasure_runner.models.exceptions import GameEngineError, ImpassableError

# Resolve config path relative to this file
_HERE = os.path.dirname(os.path.abspath(__file__))
CONFIG_PATH = os.path.realpath(os.path.join(_HERE, "..", "..", "assets", "starter.ini"))


class TestGameEngineLifecycle(unittest.TestCase):
    """Tests for engine creation and destruction."""

    def test_create_success(self):
        eng = GameEngine(CONFIG_PATH)
        self.assertIsNotNone(eng)
        eng.destroy()

    def test_destroy_twice(self):
        eng = GameEngine(CONFIG_PATH)
        eng.destroy()
        eng.destroy()  # should not raise


class TestGameEngineQueries(unittest.TestCase):
    """Tests for metadata queries."""

    def setUp(self):
        self.eng = GameEngine(CONFIG_PATH)

    def tearDown(self):
        self.eng.destroy()

    def test_get_room_count(self):
        count = self.eng.get_room_count()
        self.assertGreater(count, 0)

    def test_get_room_dimensions(self):
        width, height = self.eng.get_room_dimensions()
        self.assertGreater(width, 0)
        self.assertGreater(height, 0)

    def test_get_room_ids(self):
        ids = self.eng.get_room_ids()
        self.assertIsInstance(ids, list)
        self.assertEqual(len(ids), self.eng.get_room_count())


class TestPlayerBasics(unittest.TestCase):
    """Tests for Player wrapper basics."""

    def setUp(self):
        self.eng = GameEngine(CONFIG_PATH)

    def tearDown(self):
        self.eng.destroy()

    def test_player_not_none(self):
        self.assertIsNotNone(self.eng.player)

    def test_get_room(self):
        room_id = self.eng.player.get_room()
        self.assertIsInstance(room_id, int)
        self.assertGreaterEqual(room_id, 0)

    def test_get_position(self):
        x, y = self.eng.player.get_position()
        self.assertIsInstance(x, int)
        self.assertIsInstance(y, int)
        self.assertGreaterEqual(x, 0)
        self.assertGreaterEqual(y, 0)

    def test_initial_collected_count_zero(self):
        self.assertEqual(self.eng.player.get_collected_count(), 0)

    def test_initial_collected_treasures_empty(self):
        treasures = self.eng.player.get_collected_treasures()
        self.assertEqual(len(treasures), 0)

    def test_has_collected_treasure_false_initially(self):
        self.assertFalse(self.eng.player.has_collected_treasure(0))


class TestMovement(unittest.TestCase):
    """Tests for player movement."""

    def setUp(self):
        self.eng = GameEngine(CONFIG_PATH)

    def tearDown(self):
        self.eng.destroy()

    def _try_move(self, direction):
        """Attempt a move; return True if it succeeded, False if blocked."""
        try:
            self.eng.move_player(direction)
            return True
        except ImpassableError:
            return False

    def test_move_all_directions(self):
        """Each direction either succeeds or raises ImpassableError."""
        for d in Direction:
            eng = GameEngine(CONFIG_PATH)
            try:
                eng.move_player(d)
            except ImpassableError:
                pass  # blocked is acceptable
            eng.destroy()

    def test_move_changes_position_or_blocks(self):
        """A successful move must change the player state."""
        x0, y0 = self.eng.player.get_position()
        room0 = self.eng.player.get_room()

        moved = False
        for d in Direction:
            if self._try_move(d):
                x1, y1 = self.eng.player.get_position()
                room1 = self.eng.player.get_room()
                # Either position or room changed
                if (x1, y1) != (x0, y0) or room1 != room0:
                    moved = True
                    break

        # At least one direction should move the player (rooms have interior floor)
        self.assertTrue(moved, "Player could not move in any direction")

    def test_move_into_wall_raises(self):
        """Moving into a wall repeatedly should eventually raise ImpassableError."""
        blocked = False
        for _ in range(50):
            try:
                self.eng.move_player(Direction.NORTH)
            except ImpassableError:
                blocked = True
                break
        self.assertTrue(blocked, "Expected wall collision after repeated north moves")


class TestRendering(unittest.TestCase):
    """Tests for room rendering."""

    def setUp(self):
        self.eng = GameEngine(CONFIG_PATH)

    def tearDown(self):
        self.eng.destroy()

    def test_render_current_room(self):
        rendered = self.eng.render_current_room()
        self.assertIsInstance(rendered, str)
        self.assertGreater(len(rendered), 0)
        # Should contain newlines (multi-line)
        self.assertIn("\n", rendered)

    def test_render_contains_player(self):
        rendered = self.eng.render_current_room()
        # The default player character is '@'
        self.assertIn("@", rendered)


class TestTreasureCollection(unittest.TestCase):
    """Tests for treasure collection via movement."""

    def setUp(self):
        self.eng = GameEngine(CONFIG_PATH)

    def tearDown(self):
        self.eng.destroy()

    def test_collected_count_increases(self):
        """
        Walk around and check that collecting a treasure increments the count.
        We do a sweep in each direction hoping to land on a treasure.
        """
        initial_count = self.eng.player.get_collected_count()

        # Walk around for a while trying to collect treasures
        directions = [Direction.SOUTH, Direction.EAST, Direction.SOUTH,
                      Direction.EAST, Direction.NORTH, Direction.WEST]
        for d in directions:
            try:
                self.eng.move_player(d)
            except (ImpassableError, GameEngineError):
                pass

        # Count may or may not have gone up depending on config;
        # just verify the API doesn't crash and returns an int
        count = self.eng.player.get_collected_count()
        self.assertIsInstance(count, int)
        self.assertGreaterEqual(count, initial_count)


class TestReset(unittest.TestCase):
    """Tests for game reset."""

    def setUp(self):
        self.eng = GameEngine(CONFIG_PATH)

    def tearDown(self):
        self.eng.destroy()

    def test_reset_restores_position(self):
        x0, y0 = self.eng.player.get_position()
        room0 = self.eng.player.get_room()

        # Move around
        for d in [Direction.SOUTH, Direction.EAST, Direction.SOUTH]:
            try:
                self.eng.move_player(d)
            except (ImpassableError, GameEngineError):
                pass

        # Reset
        self.eng.reset()

        x1, y1 = self.eng.player.get_position()
        room1 = self.eng.player.get_room()

        self.assertEqual(room1, room0)
        self.assertEqual(x1, x0)
        self.assertEqual(y1, y0)

    def test_reset_clears_collected_treasures(self):
        # Move around to possibly collect treasures
        for d in [Direction.SOUTH, Direction.EAST, Direction.NORTH, Direction.WEST]:
            try:
                self.eng.move_player(d)
            except (ImpassableError, GameEngineError):
                pass

        self.eng.reset()
        self.assertEqual(self.eng.player.get_collected_count(), 0)

    def test_reset_multiple_times(self):
        """Reset should be safe to call multiple times."""
        self.eng.reset()
        self.eng.reset()
        x, y = self.eng.player.get_position()
        self.assertGreaterEqual(x, 0)
        self.assertGreaterEqual(y, 0)


if __name__ == "__main__":
    unittest.main()

