"""Player profile persistence helpers."""

from __future__ import annotations

import json
from datetime import datetime, timezone
from pathlib import Path
from typing import Callable


def _utc_now_iso() -> str:
    """Return an ISO-8601 UTC timestamp ending with Z."""
    return datetime.now(timezone.utc).replace(microsecond=0).isoformat().replace("+00:00", "Z")


def _to_int(value: object, default: int = 0) -> int:
    try:
        return int(value)
    except (TypeError, ValueError):
        return default


def normalize_profile(data: dict | None, default_name: str = "Player") -> dict:
    """Ensure profile has the required keys and valid value types."""
    source = data or {}
    player_name = str(source.get("player_name") or default_name).strip() or default_name

    profile = {
        "player_name": player_name,
        "games_played": max(_to_int(source.get("games_played"), 0), 0),
        "max_treasure_collected": max(_to_int(source.get("max_treasure_collected"), 0), 0),
        "most_rooms_world_completed": max(_to_int(source.get("most_rooms_world_completed"), 0), 0),
        "timestamp_last_played": str(source.get("timestamp_last_played") or _utc_now_iso()),
    }
    return profile


def load_profile(profile_path: str) -> dict | None:
    """Load a profile JSON file; return None when unreadable or invalid."""
    path = Path(profile_path)
    if not path.exists():
        return None

    try:
        with path.open("r", encoding="utf-8") as handle:
            data = json.load(handle)
    except (OSError, json.JSONDecodeError):
        return None

    if not isinstance(data, dict):
        return None

    return data


def save_profile(profile_path: str, profile: dict) -> None:
    """Persist profile JSON to disk."""
    path = Path(profile_path)
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", encoding="utf-8") as handle:
        json.dump(profile, handle, indent=2)
        handle.write("\n")


def load_or_create_profile(profile_path: str, request_player_name: Callable[[], str]) -> tuple[dict, bool]:
    """Load profile from disk or create a new one and prompt for player name."""
    loaded = load_profile(profile_path)
    if loaded is not None:
        return normalize_profile(loaded), False

    entered_name = (request_player_name() or "").strip() or "Player"
    profile = normalize_profile({"player_name": entered_name})
    save_profile(profile_path, profile)
    return profile, True


def update_profile_after_run(
    profile: dict,
    run_treasure_collected: int,
    run_rooms_completed: int,
    run_world_completed: bool = False,
) -> dict:
    """Update profile stats after a run (win, quit, or error)."""
    normalized = normalize_profile(profile)
    normalized["games_played"] = normalized["games_played"] + 1
    normalized["max_treasure_collected"] = max(
        normalized["max_treasure_collected"],
        run_treasure_collected,
        0,
    )
    if run_world_completed:
        # Persist room-progress on wins by advancing exactly one level.
        normalized["most_rooms_world_completed"] = (
            normalized["most_rooms_world_completed"] + 1
        )
    normalized["timestamp_last_played"] = _utc_now_iso()
    return normalized
