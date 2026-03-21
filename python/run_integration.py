#!/usr/bin/env python3
"""Deterministic system integration test runner for Treasure Runner."""

import os
import argparse
from treasure_runner.bindings import Direction
from treasure_runner.models.game_engine import GameEngine
from treasure_runner.models.exceptions import GameEngineError, ImpassableError


class IntegrationLogger:
    """Logs integration test results to a file."""

    def __init__(self, log_path: str) -> None:
        """Initialize the logger with output file path."""
        self.log_path = log_path
        self.log_file = open(log_path, 'w')
        self.step_count = 0
        self.collected_total = 0

    def log(self, message: str) -> None:
        """Write a message to the log file."""
        self.log_file.write(message + '\n')
        self.log_file.flush()

    def log_run_start(self, config_path: str) -> None:
        """Log the start of a run."""
        self.log(f"RUN_START|config={config_path}")

    def log_state(self, engine: GameEngine, phase: str, step: int) -> None:
        """Log the current game state."""
        room = engine.player.get_room()
        x, y = engine.player.get_position()
        collected = engine.player.get_collected_count()
        state_str = f"room={room}|x={x}|y={y}|collected={collected}"
        self.log(f"STATE|step={step}|phase={phase}|state={state_str}")

    def log_entry(self, direction: str) -> None:
        """Log the entry phase start with direction."""
        self.log(f"ENTRY|direction={direction}")

    def log_move(self, step: int, phase: str, direction: str, result: str,
                 before_state: dict, after_state: dict) -> None:
        """Log a move action."""
        before_str = f"room={before_state['room']}|x={before_state['x']}|y={before_state['y']}|collected={before_state['collected']}"
        after_str = f"room={after_state['room']}|x={after_state['x']}|y={after_state['y']}|collected={after_state['collected']}"
        delta_collected = after_state['collected'] - before_state['collected']
        self.log(f"MOVE|step={step}|phase={phase}|dir={direction}|result={result}|before={before_str}|after={after_str}|delta_collected={delta_collected}")

    def log_sweep_start(self, phase: str, direction: str) -> None:
        """Log the start of a sweep phase."""
        self.log(f"SWEEP_START|phase={phase}|dir={direction}")

    def log_sweep_end(self, phase: str, reason: str, moves: int) -> None:
        """Log the end of a sweep phase."""
        self.log(f"SWEEP_END|phase={phase}|reason={reason}|moves={moves}")

    def log_run_end(self, steps: int, collected_total: int) -> None:
        """Log the end of the run."""
        self.log(f"RUN_END|steps={steps}|collected_total={collected_total}")

    def log_terminated(self) -> None:
        """Log termination message."""
        self.log("TERMINATED: Initial Move Error")

    def close(self) -> None:
        """Close the log file."""
        self.log_file.close()


def get_player_state(engine: GameEngine) -> dict:
    """Get current player state as a dictionary."""
    room = engine.player.get_room()
    x, y = engine.player.get_position()
    collected = engine.player.get_collected_count()
    return {"room": room, "x": x, "y": y, "collected": collected}


def states_equal(state1: dict, state2: dict) -> bool:
    """Check if two player states are equal."""
    return (state1["room"] == state2["room"] and
            state1["x"] == state2["x"] and
            state1["y"] == state2["y"] and
            state1["collected"] == state2["collected"])


def find_entry_move(engine: GameEngine, logger: IntegrationLogger) -> Direction:
    """
    Find the first move that moves the player off the starting portal.
    
    Tries directions in order: SOUTH, WEST, NORTH, EAST.
    Accepts a direction if it stays in the same room but changes position.
    """
    directions = [Direction.SOUTH, Direction.WEST, Direction.NORTH, Direction.EAST]
    direction_names = ["SOUTH", "WEST", "NORTH", "EAST"]

    for direction, dir_name in zip(directions, direction_names):
        # Get initial state
        initial_state = get_player_state(engine)
        
        try:
            # Try the move
            engine.move_player(direction)
            moved_state = get_player_state(engine)
            
            # Check acceptance criteria:
            # - Same room
            # - Different position
            if (initial_state["room"] == moved_state["room"] and
                (initial_state["x"] != moved_state["x"] or
                 initial_state["y"] != moved_state["y"])):
                # This direction works
                engine.reset()
                return direction
            
            # Not accepted, reset and try next
            engine.reset()
        except ImpassableError:
            # This direction is blocked, try next
            engine.reset()
        except GameEngineError:
            # Other error, try next
            engine.reset()

    raise RuntimeError("Could not find valid entry move")


def execute_move(engine: GameEngine, direction: Direction, step: int, phase: str,
                 logger: IntegrationLogger) -> tuple[bool, str]:
    """
    Execute a single move and log it.
    
    Returns (success, result_reason) where:
    - success: True if move completed without error
    - result_reason: "OK", "BLOCKED", "NO_PROGRESS", or "ERROR"
    """
    direction_names = {
        Direction.SOUTH: "SOUTH",
        Direction.WEST: "WEST",
        Direction.NORTH: "NORTH",
        Direction.EAST: "EAST"
    }
    dir_name = direction_names[direction]
    
    before_state = get_player_state(engine)
    
    try:
        engine.move_player(direction)
        after_state = get_player_state(engine)
        
        # Check if position changed
        if states_equal(before_state, after_state):
            result = "NO_PROGRESS"
        else:
            result = "OK"
        
        logger.log_move(step, phase, dir_name, result, before_state, after_state)
        return True, result
    except ImpassableError:
        logger.log_move(step, phase, dir_name, "BLOCKED", before_state, before_state)
        return True, "BLOCKED"
    except GameEngineError as e:
        logger.log_move(step, phase, dir_name, "ERROR", before_state, before_state)
        return False, "ERROR"


def run_sweep(engine: GameEngine, direction: Direction, phase: str, initial_step: int,
              logger: IntegrationLogger) -> tuple[int, str]:
    """
    Run a directional sweep until a stop condition is reached.
    
    Returns (final_step, stop_reason)
    """
    direction_names = {
        Direction.SOUTH: "SOUTH",
        Direction.WEST: "WEST",
        Direction.NORTH: "NORTH",
        Direction.EAST: "EAST"
    }
    dir_name = direction_names[direction]
    
    logger.log_sweep_start(phase, dir_name)
    
    visited_states = set()
    current_step = initial_step
    moves_in_sweep = 0
    
    while True:
        current_state = get_player_state(engine)
        state_key = (current_state["room"], current_state["x"], current_state["y"], current_state["collected"])
        
        # Check for cycle
        if state_key in visited_states:
            logger.log_sweep_end(phase, "CYCLE_DETECTED", moves_in_sweep)
            return current_step, "CYCLE_DETECTED"
        
        visited_states.add(state_key)
        
        # Try to move
        current_step += 1
        success, result = execute_move(engine, direction, current_step, phase, logger)
        
        if not success:
            # Error occurred
            logger.log_sweep_end(phase, "BLOCKED", moves_in_sweep)
            return current_step, "BLOCKED"
        
        if result in ("BLOCKED", "NO_PROGRESS"):
            # Stop condition reached
            logger.log_sweep_end(phase, "BLOCKED", moves_in_sweep)
            return current_step, "BLOCKED"
        
        # result == "OK", increment moves and continue
        moves_in_sweep += 1


def parse_args():
    parser = argparse.ArgumentParser(description="Treasure Runner integration test logger")
    parser.add_argument(
        "--config",
        required=True,
        help="Path to generator config file",
    )
    parser.add_argument(
        "--log",
        required=True,
        help="Output log path",
    )
    return parser.parse_args()


def main():
    args = parse_args()
    config_path = os.path.abspath(args.config)
    log_path = os.path.abspath(args.log)
    
    logger = IntegrationLogger(log_path)
    
    try:
        # Log run start
        logger.log_run_start(config_path)
        
        # Initialize game engine
        engine = GameEngine(config_path)
        
        # Log initial state (spawn)
        logger.log_state(engine, "SPAWN", 0)
        
        # Find entry move
        entry_direction = find_entry_move(engine, logger)
        direction_names = {Direction.SOUTH: "SOUTH", Direction.WEST: "WEST", Direction.NORTH: "NORTH", Direction.EAST: "EAST"}
        logger.log_entry(direction_names[entry_direction])
        
        # Execute entry move
        current_step = 1
        success, result = execute_move(engine, entry_direction, current_step, "ENTRY", logger)
        
        if not success or result == "ERROR":
            logger.log_terminated()
            logger.log_run_end(current_step, engine.player.get_collected_count())
            logger.close()
            return 0
        
        # Run the four sweeps
        sweep_phases = [
            (Direction.SOUTH, "SWEEP_SOUTH"),
            (Direction.WEST, "SWEEP_WEST"),
            (Direction.NORTH, "SWEEP_NORTH"),
            (Direction.EAST, "SWEEP_EAST"),
        ]
        
        for direction, phase_name in sweep_phases:
            current_step, _ = run_sweep(engine, direction, phase_name, current_step, logger)
        
        # Log final state
        logger.log_state(engine, "FINAL", current_step)
        
        # Log run end
        collected_total = engine.player.get_collected_count()
        logger.log_run_end(current_step, collected_total)
        
        engine.destroy()
        logger.close()
        
        return 0
    
    except Exception as e:
        print(f"Error: {e}")
        logger.close()
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
