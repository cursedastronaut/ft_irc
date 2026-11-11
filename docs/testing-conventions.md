# Test naming and isolation (CORE-09)

Each owner adds focused fixtures in their subsystem instead of changing a
shared monolithic suite. Existing runner entry points remain unchanged.

| Kind / owner | Location | Command | Bound |
| --- | --- | --- | --- |
| Shared C++98 contracts / all owners | `tests/contracts/<behavior>.cpp` and optional `fixtures/*.cpp` | `python3 tests/contracts/run.py` | 60s compile, 10s per executable |
| Parser/identity units / Person 2 | `tests/protocol/<behavior>.cpp` | `python3 tests/protocol/run.py` | Runner compile/run timeouts |
| Transport fixtures / Person 1 | `tests/net/test_<behavior>.cpp` | `make net_test` or the focused compile/run documented in its PR | CI job maximum 5 min; new subprocess fixtures use explicit 10s run bound |
| Channel units / Person 3 | `tests/channels/<behavior>.cpp` | `python3 tests/channels/run.py` | 60s compile, 10s per executable |
| Reusable actor/permission specification | `tests/fixtures/channel_permissions.json` | `python3 -B -m unittest discover -s tests/fixtures -p 'test_*.py' -v` | CI job maximum 2 min |
| Fixture resources / all owners | `tests/support/isolation.py` | `python3 -B -m unittest discover -s tests/support -p 'test_*.py' -v` | Explicit command timeout; timeout regression uses 0.1s |
| Future real-client channel scenarios | `tests/scenarios/<behavior>.py` | Document exact command and client version in the scenario/PR | Explicit startup/read/process deadlines, never unlimited waits |

Use lowercase descriptive filenames such as `operator_departure.cpp`.
Add the fixture's source dependencies to its subsystem runner's suite map;
do not link the entire server to test a value-only operation. C++ fixtures
must still compile with `-std=c++98 -Wall -Wextra -Werror`; Python harnesses
may use standard/external tools as allowed by CONTRIBUTING. Prefer
`CHECK`-style checks that remain active with NDEBUG over disabled asserts.

## Setup and teardown

- Create one `FixtureSandbox` context per scenario. Its temporary directory
  owns logs and cwd; failure and timeout paths remove only that directory.
  `run()` preserves the child's failure status, captures stdout/stderr in
  numbered logs, and kills only its own process group if its deadline expires.
  Inspect/copy relevant redacted logs before leaving the context when needed.
- Do not use `pkill`, fixed filenames in shared `/tmp`, arbitrary PID files,
  global process-name cleanup or deletion of another fixture's directories.
  No shell is used to launch fixture commands. Fixtures must not detach child
  services from the owned process group. Never print real passwords.
- For harness listeners, bind `127.0.0.1:0` and hold the socket using
  `reserved_loopback_listener()` until it is handed off or closed. This is
  test code, not a change to ircserv's allowed CLI port range. For a child
  ircserv that must bind itself, release the reservation immediately before
  starting it and retry a bounded number of times on an address-in-use
  startup failure. A port number alone is not a reservation.
- Readiness must use bounded polling/reads rather than fixed long sleeps.
  On teardown, reap owned processes before removing their log directory.
  Never assume a failure path will reach an ordinary final cleanup line.

## Recipients and deterministic state

Load fresh actors/channels from the CORE-06 fixture for each scenario. Record
actual recipients as stable ClientIds, then compare exact sets/counts:
exclude outsiders, deduplicate shared-channel peers and detect double
delivery. Keep nickname display snapshots separate. A reused nickname or
socket number must not carry old membership or invitations into a new ID.
Use distinct roles in both channels to expose accidental global privileges.
Every issue's PR includes one normal case and one rejected/failure case.

## What a passing check establishes

The CORE-03 tests compile and demonstrate boundary stubs. CORE-06 validates
the permission **specification** and fixture integrity. Neither proves
unimplemented IRC handlers enforce the matrix. A channel unit test does not
prove live irssi interoperability. Record actual executed commands and
limitations separately from the integration gates in
`docs/integration-checkpoints.md`; real-client testing follows
`docs/reference-client.md` when its server prerequisites exist.

CI already runs subsystem C++98 suites with GCC/Clang and test-only memory
instrumentation. The added fixture workflow also checks normal cleanup,
failed-command status propagation, hung-command timeout and owned loopback
port release. A PR is not reported as hosted-CI green until it is pushed
and the workflow has actually completed.
