# Channel foundation contract proposal

## Status and dependencies

**Draft for team approval, not an approved or frozen shared contract.**
The base `test` commit `1a6cb239f53bc2161eab634753dd3b83bad64394` contains
only `.github/` templates and the root README. CORE-02 (#2), CORE-03 (#5)
and PROTO-01 (#13) have no merged contract or approved stubs in that base.
This branch prepares the requested CHAN-01 (#9), CHAN-02 (#20) and
CHAN-03 (#26) code for review in isolation. It does **not** claim those
prerequisites have been completed or that team approval was obtained.

Before merging, all three owners must approve the minimal shared types and
policy here (or replace this proposal with the approved prerequisite work).
The dependency/start gate in CONTRIBUTING is outstanding: these are draft
changes, not a claim of dependency-compliant completed work. No other CORE
issue is closed by this proposal. Person 1 must run the focused tests and
confirm no unrelated behavior changed. Any newly approved signature must be
recorded before integrating callers.

## Ownership boundaries from CORE-01

| Owner | Implementation paths | Reviewer rotation |
| --- | --- | --- |
| Person 1 | `src/net`, `src/main.cpp`, root `Makefile` | Person 2 |
| Person 2 | `src/protocol`, `src/commands/auth` | Person 3 |
| Person 3 | `src/channels`, `src/commands/channel` | Person 1 |

All three approve `include/contracts`. This branch adds minimal proposed
identity and shared casemapping headers there; it does not implement transport/protocol
interfaces or rename any frozen API. Tests are isolated in `tests/channels`;
the only CI addition is `.github/workflows/channels.yml`. The existing
templates and root README are untouched. The requested three-task archive
uses separate commits on one branch; CORE-01 otherwise prefers one issue
per branch/PR, so reviewers may split/cherry-pick these commits if required.

## Identity and model (CHAN-01)

- `irc::ClientId` is `unsigned long`, as proposed in CORE-02. Allocation,
  exhaustion checks and never-reuse guarantees belong to transport, not
  this channel model. A nickname, socket FD, pointer or iterator is never
  used as membership identity.
- `Channel` is a server-independent, copy-constructible value containing
  canonical/display names, `set<ClientId>` members/operators/invitations,
  topic and mode storage. No socket fields, I/O or callbacks occur here.
- Names are immutable after construction to prevent a registry key becoming
  inconsistent through assignment. Copy construction owns independent data;
  assignment is intentionally unavailable.
- Members/operators/invitations are raw value storage at this foundation
  layer. Permission checks, operator-subset enforcement, invitation lifecycle
  and first/last-operator behavior belong to later service/command issues.
  There is no automatic operator promotion or membership operation here.
- Proposed empty storage defaults: all mode flags false, empty topic/key,
  absent limit (`hasLimit == false`, numeric storage zero). This records a
  deterministic value initialization, not implemented MODE/TOPIC/INVITE
  command behavior or approval/completion of CHAN-06/07.

## I/O boundary (CORE-04)

Channel types do not call `poll`, `recv`, `send`, `close` or `fork`. Future
handlers must queue replies through transport rather than perform I/O.
Transport remains responsible for one readiness loop, nonblocking sockets
and return-value-only recv/send handling, never inspecting errno afterward.
This branch adds no transport implementation or syscall policy exception.

## Name policy proposal (CHAN-02 / PROTO-01)

- Support ordinary `#` and local `&` channels for the mandatory project.
  A name is 2..50 **bytes**, including its prefix. Prefix-only names are
  invalid. `+`/`!` channel types and server-mask suffixes are outside this
  mandatory subset; this is an explicit project proposal, not a claim that
  the subject fixes the length or that the full RFC grammar is implemented.
- Reject ASCII bytes 0..32 (including NUL, BEL, tab, CR, LF and space), DEL,
  comma and colon. Other bytes, including bytes >=128, are preserved.
  No trimming, truncation, Unicode folding or locale-dependent conversion.
- `irc::canonicalizeIrcName` is the **one shared helper** proposed for both
  nickname and channel lookup: ASCII A-Z map to a-z, `[` to `{`, `]` to `}`,
  backslash to `|` and `^` to `~`. Equivalences follow RFC1459-style mapping.
  The choice of lower representative is deterministic and idempotent.
  A future parser/nickname implementation must reuse this helper rather
  than introduce an independent lowercase policy.
- `isValidChannelName(name)` performs validation without mutating state.
  `canonicalizeChannelName(name, key)` returns false without changing `key`
  on invalid input. On success it writes the canonical key, including when
  input/output alias. Allocation failures propagate to the caller.
- `Channel(name)` validates before construction completes, throws
  `std::invalid_argument` on malformed input and stores both canonical key
  and original display spelling. No invalid channel can enter the registry.

References for review (the subject remains authoritative):
[RFC 1459 section 2.2](https://www.rfc-editor.org/rfc/rfc1459.html#section-2.2),
[RFC 2812 sections 1.3 and 2.2](https://www.rfc-editor.org/rfc/rfc2812.html#section-1.3).
The stricter control/delimiter rejection and supported prefix subset above
are proposed project decisions requiring all three owners' approval.

## Verification

Run `python3 tests/channels/run.py --suite model` from the repository root.
The fixture constructs an empty channel without a Server, mutates only a
test nickname lookup, checks old identities cannot be inherited by another
ID and checks copied channel state is independent. It does not establish
registration, reference-client interoperability or a working IRC server.

Tests use standard-library C++98 only. The Python runner builds in an owned
temporary directory, enforces compile/run timeouts and removes only its own
temporary artifacts. Compiler/test errors produce a nonzero status.
