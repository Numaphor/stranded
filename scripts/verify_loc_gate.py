from __future__ import annotations

import argparse
import json
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[1]
VERIFY_SCRIPT = REPO_ROOT / "scripts" / "verify_loc_reduction.py"
REPORT_ROOT = REPO_ROOT / ".omx" / "logs" / "loc-gates"
GATE_A_REMOVED_PATHS = (
    "src/viewer/runtime/room_viewer_runtime_state.cpp",
    "include/private/viewer/runtime/room_viewer_runtime_state.h",
    "src/core/minimap/minimap_layout.cpp",
    "src/core/dialog/str_bg_dialog_text.cpp",
)


@dataclass(frozen=True)
class GateConfig:
    target_max: int | None = None
    stretch_max: int | None = None
    required_drop: int | None = None
    required_absent_paths: tuple[str, ...] = ()


GATE_CONFIGS = {
    "gate-a": GateConfig(
        target_max=4100,
        stretch_max=4025,
        required_absent_paths=GATE_A_REMOVED_PATHS,
    ),
    "gate-b": GateConfig(
        required_drop=120,
        required_absent_paths=GATE_A_REMOVED_PATHS,
    ),
    "gate-c": GateConfig(
        required_drop=80,
        required_absent_paths=GATE_A_REMOVED_PATHS,
    ),
}


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Run gate-specific verification presets for the LOC-reduction plan.",
    )
    parser.add_argument("gate", choices=sorted(GATE_CONFIGS), help="Gate preset to verify.")
    parser.add_argument("--previous-loc", type=int,
                        help="Required for gate-b and gate-c drop checks.")
    parser.add_argument("--jobs", type=int, default=4,
                        help="Parallelism passed through to verify_loc_reduction.py.")
    parser.add_argument("--skip-build", action="store_true",
                        help="Skip make checks and only run static/report checks.")
    parser.add_argument("--report-dir", default=str(REPORT_ROOT),
                        help="Directory for the text and JSON reports.")
    return parser.parse_args()


def build_verify_command(args: argparse.Namespace, report_dir: Path) -> tuple[list[str], Path, Path]:
    config = GATE_CONFIGS[args.gate]
    text_path = report_dir / f"{args.gate}.txt"
    json_path = report_dir / f"{args.gate}.json"

    command = [
        sys.executable,
        str(VERIFY_SCRIPT),
        "--jobs",
        str(args.jobs),
        "--report-path",
        str(text_path),
        "--json-path",
        str(json_path),
        "--forbid-pattern",
        "runtime_state.h",
    ]

    if args.skip_build:
        command.append("--skip-build")

    if config.target_max is not None:
        command.extend(["--target-max", str(config.target_max)])
    if config.stretch_max is not None:
        command.extend(["--stretch-max", str(config.stretch_max)])
    if config.required_drop is not None:
        if args.previous_loc is None:
            raise SystemExit(f"--previous-loc is required for {args.gate}")
        command.extend(["--previous-loc", str(args.previous_loc), "--required-drop", str(config.required_drop)])

    for path_text in config.required_absent_paths:
        command.extend(["--require-absent-path", path_text])

    return command, text_path, json_path


def append_gate_checks(report_path: Path, gate: str, path_checks: list[dict[str, object]]) -> None:
    lines = ["", f"Gate-specific checks ({gate}):"]
    for result in path_checks:
        status = "PASS" if result["ok"] else "FAIL"
        lines.append(f"- {result['name']}: {status} ({result['detail']})")
    with report_path.open("a", encoding="utf-8") as report:
        report.write("\n".join(lines) + "\n")


def main() -> int:
    args = parse_args()
    report_dir = Path(args.report_dir)
    if not report_dir.is_absolute():
        report_dir = REPO_ROOT / report_dir
    report_dir.mkdir(parents=True, exist_ok=True)

    command, text_path, json_path = build_verify_command(args, report_dir)
    completed = subprocess.run(command, cwd=REPO_ROOT, text=True, check=False)

    summary = {}
    if json_path.exists():
        summary = json.loads(json_path.read_text(encoding="utf-8"))

    path_checks = summary.get("path_checks", [])
    if path_checks:
        print(f"Gate-specific checks ({args.gate}):")
        for result in path_checks:
            status = "PASS" if result["ok"] else "FAIL"
            print(f"- {result['name']}: {status} ({result['detail']})")
        append_gate_checks(text_path, args.gate, path_checks)

    overall_ok = completed.returncode == 0 and all(result["ok"] for result in path_checks)
    summary["gate"] = args.gate
    summary["gate_checks"] = path_checks
    summary["overall_ok"] = overall_ok
    json_path.write_text(json.dumps(summary, indent=2) + "\n", encoding="utf-8")
    return 0 if overall_ok else 1


if __name__ == "__main__":
    sys.exit(main())
