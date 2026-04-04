from __future__ import annotations

import argparse
import io
import json
import os
import subprocess
import sys
import time
from dataclasses import dataclass
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[1]
LOC_SUFFIXES = {".cpp", ".h", ".hpp"}
DEFAULT_SCAN_ROOTS = ("src", "include")
DEFAULT_EDGE_PATTERNS = (
    "runtime_state.h",
    "str_minimap.h",
    "str_bg_dialog.h",
    "room_viewer_runtime_systems_shared.h",
    "str_room_renderer.h",
)
MANUAL_CHECKS = (
    "Boot lands directly in the room viewer.",
    "Movement and collision still work.",
    "Door transitions still work and block movement while active.",
    "Camera turning, idle recentering, START recentering, and auto-fit distance still work.",
    "Minimap updates still work.",
    "BgDialog still opens and advances with A.",
    "Paintings and NPC sprites still render correctly.",
    "If visuals changed, capture native mGBA F12 screenshots before and after for parity.",
)


@dataclass(frozen=True)
class CheckResult:
    name: str
    ok: bool
    detail: str


@dataclass(frozen=True)
class CommandResult:
    label: str
    ok: bool
    command: tuple[str, ...]
    output: str
    duration_seconds: float


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Run the LOC-reduction verification checks described in the ralplan.",
    )
    parser.add_argument("--baseline-loc", type=int, default=4340,
                        help="Baseline LOC used for delta reporting (default: 4340).")
    parser.add_argument("--previous-loc", type=int,
                        help="Previous gate LOC for required-drop checks.")
    parser.add_argument("--required-drop", type=int,
                        help="Minimum LOC drop required versus --previous-loc.")
    parser.add_argument("--target-max", type=int,
                        help="Hard maximum LOC target for the current gate.")
    parser.add_argument("--stretch-max", type=int,
                        help="Stretch maximum LOC target for the current gate.")
    parser.add_argument("--jobs", type=int, default=4,
                        help="Parallelism for make commands (default: 4).")
    parser.add_argument("--skip-build", action="store_true",
                        help="Skip make -jN and make -B -jN.")
    parser.add_argument("--scan-pattern", action="append", default=[],
                        help="Extra include-edge pattern to scan for (repeatable).")
    parser.add_argument("--forbid-pattern", action="append", default=[],
                        help="Pattern that must not appear in src/include (repeatable).")
    parser.add_argument("--require-absent-path", action="append", default=[],
                        help="Repository-relative path that must not exist (repeatable).")
    parser.add_argument("--require-present-path", action="append", default=[],
                        help="Repository-relative path that must exist (repeatable).")
    parser.add_argument("--report-path",
                        help="Optional path to write the full text report.")
    parser.add_argument("--json-path",
                        help="Optional path to write a machine-readable JSON summary.")
    return parser.parse_args()


def detect_toolchain_env() -> tuple[dict[str, str], str]:
    env = os.environ.copy()
    if env.get("DEVKITARM"):
        return env, f"DEVKITARM={env['DEVKITARM']}"
    if env.get("WONDERFUL_TOOLCHAIN"):
        return env, f"WONDERFUL_TOOLCHAIN={env['WONDERFUL_TOOLCHAIN']}"

    wonderful = Path("/opt/wonderful")
    if wonderful.is_dir():
        env["WONDERFUL_TOOLCHAIN"] = str(wonderful)
        return env, f"WONDERFUL_TOOLCHAIN={wonderful} (auto-detected)"

    devkitarm_candidates = (
        Path("/opt/devkitpro/devkitARM"),
        Path.home() / "devkitPro" / "devkitARM",
    )
    for candidate in devkitarm_candidates:
        if candidate.is_dir():
            env["DEVKITARM"] = str(candidate)
            return env, f"DEVKITARM={candidate} (auto-detected)"

    return env, "none"


def count_loc() -> int:
    total = 0
    for root_name in DEFAULT_SCAN_ROOTS:
        root = REPO_ROOT / root_name
        for path in root.rglob("*"):
            if path.is_file() and path.suffix in LOC_SUFFIXES:
                with path.open("r", encoding="utf-8") as source:
                    total += sum(1 for _ in source)
    return total


def scan_patterns(patterns: tuple[str, ...]) -> dict[str, list[str]]:
    matches = {pattern: [] for pattern in patterns}
    for root_name in DEFAULT_SCAN_ROOTS:
        root = REPO_ROOT / root_name
        for path in root.rglob("*"):
            if not path.is_file() or path.suffix not in LOC_SUFFIXES:
                continue
            with path.open("r", encoding="utf-8") as source:
                for number, line in enumerate(source, start=1):
                    for pattern in patterns:
                        if pattern in line:
                            rel_path = path.relative_to(REPO_ROOT)
                            matches[pattern].append(f"{rel_path}:{number}: {line.rstrip()}")
    return matches


def run_command(label: str, command: tuple[str, ...], env: dict[str, str]) -> CommandResult:
    start = time.monotonic()
    completed = subprocess.run(
        command,
        cwd=REPO_ROOT,
        env=env,
        text=True,
        capture_output=True,
        check=False,
    )
    output = (completed.stdout + completed.stderr).strip()
    if not output:
        output = "<no output>"
    return CommandResult(
        label=label,
        ok=completed.returncode == 0,
        command=command,
        output=output,
        duration_seconds=time.monotonic() - start,
    )


def summarize_checks(loc_total: int, args: argparse.Namespace, pattern_matches: dict[str, list[str]]) -> list[CheckResult]:
    checks = [
        CheckResult("loc_total", True, str(loc_total)),
        CheckResult("delta_vs_baseline", True, str(loc_total - args.baseline_loc)),
    ]

    if args.target_max is not None:
        checks.append(CheckResult(
            name=f"target_max_{args.target_max}",
            ok=loc_total <= args.target_max,
            detail=f"current={loc_total}",
        ))

    if args.stretch_max is not None:
        checks.append(CheckResult(
            name=f"stretch_max_{args.stretch_max}",
            ok=loc_total <= args.stretch_max,
            detail=f"current={loc_total}",
        ))

    if args.required_drop is not None:
        if args.previous_loc is None:
            checks.append(CheckResult(
                name=f"required_drop_{args.required_drop}",
                ok=False,
                detail="--previous-loc is required",
            ))
        else:
            actual_drop = args.previous_loc - loc_total
            checks.append(CheckResult(
                name=f"required_drop_{args.required_drop}",
                ok=actual_drop >= args.required_drop,
                detail=f"drop={actual_drop} (previous={args.previous_loc}, current={loc_total})",
            ))

    for pattern in args.forbid_pattern:
        matches = pattern_matches.get(pattern, [])
        checks.append(CheckResult(
            name=f"forbid_{pattern}",
            ok=not matches,
            detail="0 matches" if not matches else f"{len(matches)} matches",
        ))

    return checks


def summarize_path_checks(args: argparse.Namespace) -> list[CheckResult]:
    checks: list[CheckResult] = []
    for path_text in args.require_absent_path:
        path = REPO_ROOT / path_text
        checks.append(CheckResult(
            name=f"require_absent_{path_text}",
            ok=not path.exists(),
            detail="missing" if not path.exists() else "still present",
        ))
    for path_text in args.require_present_path:
        path = REPO_ROOT / path_text
        checks.append(CheckResult(
            name=f"require_present_{path_text}",
            ok=path.exists(),
            detail="present" if path.exists() else "missing",
        ))
    return checks


def print_command_results(results: list[CommandResult]) -> None:
    for result in results:
        status = "PASS" if result.ok else "FAIL"
        command_text = " ".join(result.command)
        print(f"- {result.label}: {status} ({command_text}; {result.duration_seconds:.2f}s)")
        for line in result.output.splitlines():
            print(f"    {line}")


def print_check_results(results: list[CheckResult]) -> None:
    for result in results:
        status = "PASS" if result.ok else "FAIL"
        print(f"- {result.name}: {status} ({result.detail})")


def print_pattern_matches(patterns: tuple[str, ...], matches: dict[str, list[str]]) -> None:
    print("Include-edge scan:")
    for pattern in patterns:
        pattern_matches = matches[pattern]
        print(f"- {pattern}: {len(pattern_matches)} match(es)")
        for entry in pattern_matches[:20]:
            print(f"    {entry}")
        if len(pattern_matches) > 20:
            print(f"    ... {len(pattern_matches) - 20} more")


def build_json_summary(
    toolchain_note: str,
    loc_total: int,
    args: argparse.Namespace,
    command_results: list[CommandResult],
    check_results: list[CheckResult],
    path_check_results: list[CheckResult],
    pattern_matches: dict[str, list[str]],
    overall_ok: bool,
) -> dict[str, object]:
    return {
        "toolchain": toolchain_note,
        "baseline_loc": args.baseline_loc,
        "previous_loc": args.previous_loc,
        "required_drop": args.required_drop,
        "target_max": args.target_max,
        "stretch_max": args.stretch_max,
        "loc_total": loc_total,
        "overall_ok": overall_ok,
        "build_checks": [
            {
                "label": result.label,
                "ok": result.ok,
                "command": list(result.command),
                "duration_seconds": round(result.duration_seconds, 3),
                "output": result.output,
            }
            for result in command_results
        ],
        "loc_checks": [
            {
                "name": result.name,
                "ok": result.ok,
                "detail": result.detail,
            }
            for result in check_results
        ],
        "path_checks": [
            {
                "name": result.name,
                "ok": result.ok,
                "detail": result.detail,
            }
            for result in path_check_results
        ],
        "include_edge_matches": pattern_matches,
        "manual_validation_checklist": list(MANUAL_CHECKS),
    }


def write_optional_output(path_text: str | None, content: str) -> None:
    if not path_text:
        return
    path = Path(path_text)
    if not path.is_absolute():
        path = REPO_ROOT / path
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(content, encoding="utf-8")


def main() -> int:
    args = parse_args()
    env, toolchain_note = detect_toolchain_env()
    scan_patterns_list = tuple(DEFAULT_EDGE_PATTERNS + tuple(args.scan_pattern) + tuple(args.forbid_pattern))

    command_results: list[CommandResult] = []
    if not args.skip_build:
        jobs = str(args.jobs)
        command_results.append(run_command("make_incremental", ("make", f"-j{jobs}"), env))
        command_results.append(run_command("make_rebuild", ("make", "-B", f"-j{jobs}"), env))

    loc_total = count_loc()
    pattern_matches = scan_patterns(scan_patterns_list)
    check_results = summarize_checks(loc_total, args, pattern_matches)
    path_check_results = summarize_path_checks(args)

    build_ok = all(result.ok for result in command_results)
    checks_ok = all(result.ok for result in check_results)
    path_checks_ok = all(result.ok for result in path_check_results)
    overall_ok = build_ok and checks_ok and path_checks_ok

    report = io.StringIO()
    original_stdout = sys.stdout
    try:
        sys.stdout = report
        print("Toolchain:", toolchain_note)
        print("Build checks:")
        if command_results:
            print_command_results(command_results)
        else:
            print("- skipped")

        print("LOC checks:")
        print_check_results(check_results)
        if path_check_results:
            print("Path checks:")
            print_check_results(path_check_results)
        print_pattern_matches(scan_patterns_list, pattern_matches)

        print("Manual validation checklist:")
        for item in MANUAL_CHECKS:
            print(f"- [ ] {item}")
    finally:
        sys.stdout = original_stdout

    report_text = report.getvalue()
    print(report_text, end="")
    write_optional_output(args.report_path, report_text)

    json_summary = build_json_summary(
        toolchain_note=toolchain_note,
        loc_total=loc_total,
        args=args,
        command_results=command_results,
        check_results=check_results,
        path_check_results=path_check_results,
        pattern_matches=pattern_matches,
        overall_ok=overall_ok,
    )
    if args.json_path:
        write_optional_output(args.json_path, json.dumps(json_summary, indent=2) + "\n")

    return 0 if overall_ok else 1


if __name__ == "__main__":
    sys.exit(main())
