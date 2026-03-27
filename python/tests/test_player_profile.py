"""Unit tests for player profile update rules."""

import unittest

from treasure_runner.models.player_profile import update_profile_after_run


class TestPlayerProfileUpdateRules(unittest.TestCase):
    """Validate run-summary profile update behavior."""

    def test_most_rooms_not_updated_when_world_not_completed(self):
        profile = {
            "player_name": "Ada",
            "games_played": 5,
            "max_treasure_collected": 10,
            "most_rooms_world_completed": 3,
            "timestamp_last_played": "2026-01-01T00:00:00Z",
        }

        updated = update_profile_after_run(
            profile,
            run_treasure_collected=8,
            run_rooms_completed=7,
            run_world_completed=False,
        )

        self.assertEqual(updated["games_played"], 6)
        self.assertEqual(updated["max_treasure_collected"], 10)
        self.assertEqual(updated["most_rooms_world_completed"], 3)

    def test_most_rooms_updates_when_world_completed(self):
        profile = {
            "player_name": "Ada",
            "games_played": 1,
            "max_treasure_collected": 2,
            "most_rooms_world_completed": 4,
            "timestamp_last_played": "2026-01-01T00:00:00Z",
        }

        updated = update_profile_after_run(
            profile,
            run_treasure_collected=5,
            run_rooms_completed=6,
            run_world_completed=True,
        )

        self.assertEqual(updated["games_played"], 2)
        self.assertEqual(updated["max_treasure_collected"], 5)
        self.assertEqual(updated["most_rooms_world_completed"], 6)


if __name__ == "__main__":
    unittest.main()
