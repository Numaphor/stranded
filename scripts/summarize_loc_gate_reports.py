from __future__ import annotations

import argparse
import json
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[1]
DEFAULT_REPORT_ROOT = REPO_ROOT / ".omx" / "logs" / "loc-gates"


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Summarize gate verification JSON reports into a concise text or markdown view.",
    )
    parser.add_argument(
        "reports",
        nargs="*",
        help="JSON report paths. Defaults to .omx/logs/loc-gates/*.json if omitted.",
    )
    parser.add_argument("--markdown", action="store_true", help="Emit a markdown table.")
    parser.add_argument("--output", help="Optional output file path for the rendered summary.")
    return parser.parse_args()


def resolve_report_paths(args: argparse.Namespace) -> list[Path]:
    if args.reports:
        paths = [Path(report) for report in args.reports]
    else:
        paths = sorted(DEFAULT_REPORT_ROOT.glob("*.json"))
    resolved = []
    for path in paths:
        resolved.append(path if path.is_absolute() else REPO_ROOT / path)
    return [path for path in resolved if path.exists()]


def load_reports(paths: list[Path]) -> list[dict[str, object]]:
    reports: list[dict[str, object]] = []
    for path in paths:
        payload = json.loads(path.read_text(encoding="utf-8"))
        payload["_path"] = str(path.relative_to(REPO_ROOT))
        reports.append(payload)
    return reports


def summarize_report(report: dict[str, object]) -> dict[str, object]:
    build_checks = report.get("build_checks", [])
    loc_checks = {item["name"]: item for item in report.get("loc_checks", [])}
    path_checks = report.get("path_checks", [])
    gate = report.get("gate", Path(report["_path"]).stem)
    failed_paths = [item["name"] for item in path_checks if not item["ok"]]
    return {
        "gate": gate,
        "path": report["_path"],
        "overall_ok": report.get("overall_ok", False),
        "loc_total": report.get("loc_total", "?"),
        "previous_loc": report.get("previous_loc"),
        "target_max": report.get("target_max"),
        "stretch_max": report.get("stretch_max"),
        "required_drop": report.get("required_drop"),
        "build_ok": all(item["ok"] for item in build_checks),
        "path_ok": all(item["ok"] for item in path_checks),
        "forbid_runtime_state": loc_checks.get("forbid_runtime_state.h", {}).get("ok"),
        "failed_paths": failed_paths,
    }


def annotate_deltas(rows: list[dict[str, object]]) -> list[dict[str, object]]:
    previous_loc: int | None = None
    for row in rows:
        loc_total = row["loc_total"]
        if isinstance(loc_total, int) and previous_loc is not None:
            row["delta_from_previous"] = loc_total - previous_loc
        else:
            row["delta_from_previous"] = None
        if isinstance(loc_total, int):
            previous_loc = loc_total
    return rows


def render_text(rows: list[dict[str, object]]) -> str:
    lines = []
    overall_ok = all(row["overall_ok"] for row in rows) if rows else False
    lines.append(f"Overall: {'PASS' if overall_ok else 'FAIL'}")
    for row in rows:
        bits = [
            f"gate={row['gate']}",
            f"status={'PASS' if row['overall_ok'] else 'FAIL'}",
            f"loc={row['loc_total']}",
            f"build={'PASS' if row['build_ok'] else 'FAIL'}",
            f"paths={'PASS' if row['path_ok'] else 'FAIL'}",
            f"runtime_state={'PASS' if row['forbid_runtime_state'] else 'FAIL'}",
        ]
        if row["delta_from_previous"] is not None:
            bits.append(f"delta_from_previous={row['delta_from_previous']}")
        if row["target_max"] is not None:
            bits.append(f"target_max={row['target_max']}")
        if row["stretch_max"] is not None:
            bits.append(f"stretch_max={row['stretch_max']}")
        if row["required_drop"] is not None:
            bits.append(f"required_drop={row['required_drop']}")
        if row["failed_paths"]:
            bits.append(f"failed_paths={','.join(row['failed_paths'])}")
        bits.append(f"report={row['path']}")
        lines.append("- " + "; ".join(bits))
    return "\n".join(lines) + "\n"


def render_markdown(rows: list[dict[str, object]]) -> str:
    header = "| Gate | Status | LOC | Δ prev | Build | Paths | runtime_state.h | Notes | Report |"
    divider = "| --- | --- | ---: | ---: | --- | --- | --- | --- | --- |"
    body = []
    for row in rows:
        notes = []
        if row["target_max"] is not None:
            notes.append(f"target≤{row['target_max']}")
        if row["stretch_max"] is not None:
            notes.append(f"stretch≤{row['stretch_max']}")
        if row["required_drop"] is not None:
            notes.append(f"drop≥{row['required_drop']}")
        if row["failed_paths"]:
            notes.append("missing deletions")
        body.append(
            "| {gate} | {status} | {loc} | {delta} | {build} | {paths} | {runtime} | {notes} | `{report}` |".format(
                gate=row["gate"],
                status="PASS" if row["overall_ok"] else "FAIL",
                loc=row["loc_total"],
                delta=row["delta_from_previous"] if row["delta_from_previous"] is not None else "-",
                build="PASS" if row["build_ok"] else "FAIL",
                paths="PASS" if row["path_ok"] else "FAIL",
                runtime="PASS" if row["forbid_runtime_state"] else "FAIL",
                notes=", ".join(notes) or "-",
                report=row["path"],
            )
        )
    return "\n".join([header, divider, *body]) + "\n"


def write_output(output_path: str | None, content: str) -> None:
    if not output_path:
        return
    path = Path(output_path)
    if not path.is_absolute():
        path = REPO_ROOT / path
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(content, encoding="utf-8")


def main() -> int:
    args = parse_args()
    report_paths = resolve_report_paths(args)
    if not report_paths:
        raise SystemExit("No gate report JSON files found.")

    rows = annotate_deltas([summarize_report(report) for report in load_reports(report_paths)])
    content = render_markdown(rows) if args.markdown else render_text(rows)
    print(content, end="")
    write_output(args.output, content)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
