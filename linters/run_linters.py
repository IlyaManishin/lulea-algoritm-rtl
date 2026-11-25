"""Run one C++ linter against a component of the Meson build."""

import argparse
import json
from pathlib import Path
import shlex
import subprocess
import sys
from tempfile import TemporaryDirectory
from typing import Any

CPPCHECK_FLAGS = [
    "--enable=all",
    "--inconclusive",
    "--library=posix",
    "--platform=unix64",
    "--check-level=exhaustive",
    "--suppress=missingIncludeSystem",
    "--suppress=unusedFunction",
    "--suppress=checkersReport",
    "--error-exitcode=1",
]


def read_commands(
    database: Path, scope: Path, tool: str,
) -> tuple[list[dict[str, Any]], list[Path]]:
    """Select translation units owned by the component and retain their working directories."""
    entries = json.loads(database.read_text(encoding="utf-8"))
    commands: list[dict[str, Any]] = []
    sources: set[Path] = set()

    for entry in entries:
        source = (Path(entry["directory"]) / entry["file"]).resolve()
        if not source.is_relative_to(scope):
            continue

        if tool == "clang-tidy":
            # Clang cannot interpret GCC's static analyzer flag.
            if "arguments" in entry:
                entry["arguments"] = [arg for arg in entry["arguments"] if arg != "-fanalyzer"]
            else:
                args = shlex.split(entry["command"])
                entry["command"] = shlex.join(arg for arg in args if arg != "-fanalyzer")

        commands.append(entry)
        sources.add(source)

    if not commands:
        raise ValueError(f"No compilation commands found for {scope}")
    return commands, sorted(sources)


def run_linter(build_dir: Path, scope: Path, tool: str, clang_tidy_config: Path) -> int:
    """Run the linter against a temporary, component-specific compilation database."""
    commands, sources = read_commands(build_dir / "compile_commands.json", scope.resolve(), tool)
    print(f"{tool}: {len(sources)} source files in {scope}", flush=True)
    with TemporaryDirectory(prefix="lulea-lint-") as scratch:
        database = Path(scratch) / "compile_commands.json"
        database.write_text(json.dumps(commands), encoding="utf-8")
        if tool == "cppcheck":
            command = [tool, f"--project={database}", *CPPCHECK_FLAGS]
        else:
            command = [
                tool, "-p", scratch, f"--config-file={clang_tidy_config}", *map(str, sources),
            ]
        return subprocess.run(command, check=False).returncode


def main() -> int:
    """Parse the arguments passed by the component's Meson target."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build-dir", type=Path, required=True)
    parser.add_argument("--scope", type=Path, required=True)
    parser.add_argument("--tool", choices=("cppcheck", "clang-tidy"), required=True)
    parser.add_argument("--clang-tidy-config", type=Path, required=True)
    args = parser.parse_args()

    try:
        return run_linter(args.build_dir, args.scope, args.tool, args.clang_tidy_config)
    except (OSError, ValueError) as error:
        print(f"{parser.prog}: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
