"""
Backwards compatibility module for game UI.

GameUI and related entry points have been moved to the ui layer.
This module re-exports them for backwards compatibility.
"""

from ..ui.game_ui import GameUI, run_game, main, launch

__all__ = ["GameUI", "run_game", "main", "launch"]

