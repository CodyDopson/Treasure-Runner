import argparse
import os
import sys
from pathlib import Path


def _adjust_sys_path() -> None:
    """Ensure the local project paths are importable when run as a script."""
    here = Path(__file__).resolve().parent
    project_root = here.parent

    candidates = [str(here), str(project_root)]
    for candidate in reversed(candidates):
        if candidate not in sys.path:
            sys.path.insert(0, candidate)


def parse_args():
    parser = argparse.ArgumentParser(description="Treasure Runner game launcher")
    parser.add_argument(
        "--config",
        required=True,
        help="Path to world/config .ini file",
    )
    parser.add_argument(
        "--profile",
        required=True,
        help="Path to player profile file",
    )
    return parser.parse_args()


def _launch_game_ui(config_path: str, profile_path: str) -> int:
    """Launch game_ui using its available entry-point function."""
    from treasure_runner.models import game_ui

    for name in ("run_game", "main", "launch"):
        entry_point = getattr(game_ui, name, None)
        if callable(entry_point):
            try:
                result = entry_point(config_path, profile_path)
            except TypeError:
                result = entry_point(config_path)
            return int(result) if isinstance(result, int) else 0

    raise RuntimeError(
        "No callable game_ui entry point found. Expected run_game, main, or launch."
    )


def main() -> int:
    _adjust_sys_path()
    args = parse_args()

    config_path = os.path.abspath(args.config)
    profile_path = os.path.abspath(args.profile)

    try:
        return _launch_game_ui(config_path, profile_path)
    except Exception as exc:
        print(f"Error: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())