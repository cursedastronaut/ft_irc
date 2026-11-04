# Channel foundation tests

Run from the repository root:

```sh
python3 tests/channels/run.py --suite model
python3 tests/channels/run.py --suite names
python3 tests/channels/run.py --suite registry
python3 tests/channels/run.py
python3 tests/channels/run.py --sanitizers
CXX=clang++ python3 tests/channels/run.py
```

Only a C++98 compiler and Python 3 are needed; no root Makefile, running
server, ports, client or external production library is required. Tests
compile with `-std=c++98 -Wall -Wextra -Werror -pedantic-errors`.
`CXX` and optional `CXXFLAGS` configure the compiler; sanitizer instrumentation
is test-only. Each compilation has a 60-second timeout and each executable
a 10-second timeout. Every invocation owns a unique temporary directory
which is removed on success and ordinary failure; no shared processes or
files are killed/deleted.

## Cases

| Suite | Normal case | Failure/negative case |
| --- | --- | --- |
| `model` / CHAN-01 (#9) | Empty channel and independent copied state | A reused nickname on a different stable ID does not inherit membership/operator/invitation state |
| `names` / CHAN-02 (#20) | Equivalent case/special spellings and retained display spelling; exact 50-byte boundary | Empty/prefix-only/bad-prefix names, embedded NUL, delimiters, controls and overlong names are rejected without overwriting an output key |
| `registry` / CHAN-03 (#26) | Aliases return one object; empty deletion/recreation resets topic/modes/invitations without affecting another channel | Missing lookups do not insert; malformed creation is rejected; a channel with any remaining member cannot be deleted |

Registry fixtures also check const lookup and pointer stability across other
channel insertions/deletions. Returned pointers must never be used after
their channel is erased or the registry is destroyed.

Checks remain active with `-DNDEBUG` and a failure prints its file/line and
returns nonzero. Fixtures use stable IDs rather than socket numbers. This
is focused unit verification, not proof of live reference-client compatibility.
See `docs/team-contract.md` for the prerequisite approval gate.

GitHub Actions runs this same runner with GCC and Clang on Linux, Clang on
macOS and a Linux Clang AddressSanitizer/UndefinedBehaviorSanitizer job.
Hosted CI runs only after these commits are pushed or a PR is opened.

In a traced/sandboxed local environment where LeakSanitizer cannot inspect
`/proc`, run `python3 tests/channels/run.py --sanitizers --no-leak-check`.
This retains address/undefined-behavior checks but does **not** verify leaks.
The CI command deliberately keeps leak detection enabled.
