import json
import os
import sys

import przydzielaczka_py as solver


def on_solution(solution: dict) -> None:
    assignments = solution.get("solutions", [])
    print(f"  [callback] solution found — {len(assignments)} assignment(s)")


def main() -> None:
    config = solver.Config()
    config.max_solutions = 5
    config.max_runtime   = 30        # seconds
    config.verbose       = False
    config.solution_callback = on_solution

    runner = solver.SolverRunner(config)

    # --- Run from file ---
    sample = os.path.join(os.path.dirname(__file__), "../../../tests/data/classes_2526_L_ISI.json")
    if os.path.exists(sample):
        print("Running solver from file...")
        result = runner.run_file(sample)
        meta   = result.get("meta", {})
        print(f"  solutions : {len(result.get('solutions', []))}")
        print(f"  duration  : {meta.get('duration_ms', '?')} ms")
        print(json.dumps(result, indent=2, ensure_ascii=False))
    else:
        print(f"Sample file not found at {sample!r}, skipping file example.")

    # # --- Run from dict ---
    # print("\nRunning solver from dict (empty input)...")
    # empty_input = {"constraints": [], "classes": []}
    # result = runner.run(empty_input)
    # print(f"  solutions : {len(result.get('solutions', []))}")
    # print(json.dumps(result, indent=2, ensure_ascii=False))
    #
    # # --- Run from JSON string ---
    # print("\nRunning solver from JSON string...")
    # result = runner.run(json.dumps(empty_input))
    # print(f"  solutions : {len(result.get('solutions', []))}")


if __name__ == "__main__":
    main()