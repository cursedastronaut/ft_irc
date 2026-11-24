#!/usr/bin/env python3
"""Build/run isolated C++98 network fixtures; no production build required."""

import argparse
import os
from pathlib import Path
import shlex
import subprocess
import sys
import tempfile


ROOT = Path(__file__).resolve().parents[2]
NET_CORE_SRCS = [
	"src/net/ClientRecord.cpp",
	"src/net/parsers.cpp",
	"src/net/ScopedShutdownGuard.cpp",
	"src/net/ScopedSocket.cpp",
	"src/net/ServerConfig.cpp",
]
SUITES = {
	"config": NET_CORE_SRCS,
	"socket": NET_CORE_SRCS,
	"read_lines": NET_CORE_SRCS,
	"net_output": NET_CORE_SRCS,
	"disconnect_order": NET_CORE_SRCS,
	"flush_before_close": NET_CORE_SRCS,
	"shutdown": NET_CORE_SRCS,
	"multi_client_smoke": NET_CORE_SRCS,
	"fragmentation_smoke": NET_CORE_SRCS,
	"shutdown_signals": NET_CORE_SRCS,
	"work_budgets": NET_CORE_SRCS,
	"poll_error_policy": NET_CORE_SRCS,
}


def main():
	parser = argparse.ArgumentParser(description=__doc__)
	parser.add_argument("--suite", choices=sorted(SUITES))
	parser.add_argument("--sanitizers", action="store_true")
	parser.add_argument("--no-leak-check", action="store_true",
						help="disable LeakSanitizer only in restricted local environments")
	args = parser.parse_args()
	if args.no_leak_check and not args.sanitizers:
		parser.error("--no-leak-check requires --sanitizers")
	compiler = shlex.split(os.environ.get("CXX", "c++"))
	flags = shlex.split(os.environ.get("CXXFLAGS", ""))
	flags += ["-std=c++98", "-Wall", "-Wextra", "-Werror", "-pedantic-errors"]
	flags += ["-I", str(ROOT / "include"), "-I", str(ROOT / "src")]
	environment = os.environ.copy()
	if args.sanitizers:
		flags += ["-g", "-O1", "-fsanitize=address,undefined", "-fno-omit-frame-pointer"]
		leak_check = "0" if args.no_leak_check else "1"
		environment["ASAN_OPTIONS"] = "detect_leaks=" + leak_check + ":halt_on_error=1"
		environment["UBSAN_OPTIONS"] = "halt_on_error=1:print_stacktrace=1"
	suites = [args.suite] if args.suite else sorted(SUITES)
	try:
		with tempfile.TemporaryDirectory(prefix="ft-irc-net-") as temporary:
			for suite in suites:
				executable = Path(temporary) / suite
				sources = [str(ROOT / source) for source in SUITES[suite]]
				sources.append(str(ROOT / "tests" / "net" / ("test_" + suite + ".cpp")))
				print("BUILD " + suite + " (" + " ".join(compiler) + ")", flush=True)
				subprocess.run(compiler + flags + sources + ["-o", str(executable)],
							   check=True, timeout=60, cwd=ROOT)
				subprocess.run([str(executable)], check=True, timeout=10,
							   cwd=temporary, env=environment)
	except (OSError, subprocess.SubprocessError) as error:
		print("Network tests failed: " + str(error), file=sys.stderr)
		return 1
	return 0


if __name__ == "__main__":
	sys.exit(main())