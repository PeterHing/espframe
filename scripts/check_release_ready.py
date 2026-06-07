#!/usr/bin/env python3
"""Run the local release-readiness gate and report the result clearly."""

from __future__ import annotations

import subprocess
import sys


def run(command: list[str], label: str) -> bool:
    print(f"[RUN] {label}")
    result = subprocess.run(command)
    if result.returncode != 0:
        print(f"[FAIL] {label}")
        return False
    print(f"[PASS] {label}")
    return True


def git_clean() -> bool:
    result = subprocess.run(
        ["git", "status", "--short"],
        check=False,
        capture_output=True,
        text=True,
    )
    if result.returncode != 0:
        print("[FAIL] Git working tree check")
        return False
    if result.stdout.strip():
        print("[FAIL] Git working tree check")
        print(result.stdout.rstrip())
        return False
    print("[PASS] Git working tree is clean")
    return True


def main() -> int:
    print("Espframe release-readiness check")
    print("This runs the normal local gate. Run ESPHome compile separately before firmware releases.")
    checks = [
        run(["npm", "run", "check:all"], "Local validation gate"),
        git_clean(),
    ]
    if all(checks):
        print("[PASS] Release-readiness checks passed")
        return 0
    print("[FAIL] Release-readiness checks failed")
    return 1


if __name__ == "__main__":
    raise SystemExit(main())
