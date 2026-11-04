#!/usr/bin/env python3
"""Build/run isolated C++98 channel fixtures; no production build required."""

import argparse
import os
from pathlib import Path
import shlex
import subprocess
import sys
import tempfile


ROOT = Path(__file__).resolve().parents[2]
SUITES = {
    "model": ["src/channels/Channel.cpp"],
}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--suite", choices=sorted(SUITES))
    parser.add_argument("--sanitizers", action="store_true")
    args = parser.parse_args()
    compiler = shlex.split(os.environ.get("CXX", "c++"))
    flags = shlex.split(os.environ.get("CXXFLAGS", ""))
    flags += ["-std=c++98", "-Wall", "-Wextra", "-Werror", "-pedantic-errors"]
    flags += ["-I", str(ROOT / "include"), "-I", str(ROOT / "src")]
    environment = os.environ.copy()
    if args.sanitizers:
        flags += ["-g", "-O1", "-fsanitize=address,undefined", "-fno-omit-frame-pointer"]
        environment["ASAN_OPTIONS"] = "detect_leaks=1:halt_on_error=1"
        environment["UBSAN_OPTIONS"] = "halt_on_error=1:print_stacktrace=1"
    suites = [args.suite] if args.suite else sorted(SUITES)
    try:
        with tempfile.TemporaryDirectory(prefix="ft-irc-channels-") as temporary:
            for suite in suites:
                executable = Path(temporary) / suite
                sources = [str(ROOT / source) for source in SUITES[suite]]
                sources.append(str(ROOT / "tests" / "channels" / (suite + ".cpp")))
                print("BUILD " + suite + " (" + " ".join(compiler) + ")", flush=True)
                subprocess.run(compiler + flags + sources + ["-o", str(executable)],
                               check=True, timeout=60, cwd=ROOT)
                subprocess.run([str(executable)], check=True, timeout=10,
                               cwd=temporary, env=environment)
    except (OSError, subprocess.SubprocessError) as error:
        print("Channel tests failed: " + str(error), file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
