# Channel foundation contract proposal

## Status and dependencies

This CORE-03 update is based on `test` at
`eca2d470e9c7ed6d2e17733498a0ba80d19d10ae`. CORE-01 ownership rules,
CORE-02's Protocol.hpp, the reference-client document and PROTO-01 grammar
are already present in that base, even where the issue tracker remains open.
The earlier CHAN-01/02/03 foundation code is merged and retained.

**New CORE-03 signatures and policy decisions below are submitted for review,
not represented as already approved.** All three owners must approve shared
contract additions before merge. Person 1 is the requested reviewer.
No Person 2 implementation is supplied or changed by this PR: a read-only
provider interface and separately compiled test doubles establish the boundary.
Approval of a contract is distinct from implementing its real provider.

## Ownership boundaries from CORE-01

| Owner | Implementation paths | Reviewer rotation |
| --- | --- | --- |
| Person 1 | `src/net`, `src/main.cpp`, root `Makefile` | Person 2 |
| Person 2 | `src/protocol`, `src/commands/auth` | Person 3 |
| Person 3 | `src/channels`, `src/commands/channel` | Person 1 |

All three approve `include/contracts`. The identity, casemapping and
transport/protocol headers are merged prerequisites. This PR adds the
ChannelService/IdentityLookup boundary without renaming those existing
interfaces. Contract fixtures are in `tests/contracts`; permission fixtures
and resource checks are in `tests/fixtures` and `tests/support`. Existing
contract CI discovers the new C++ suite; the additional fixture workflow
runs the Python checks. The templates and root README are untouched.
The requested grouped delivery uses task-specific commits; each task keeps
its original half-day-or-less scope. Shared approval is a merge gate, not
permission to silently implement another person's provider.

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

## Message framing and limits (PROTO-01)

- **Line length:** 512 bytes maximum, **including** the trailing terminator
  (RFC 2812 standard). Lines exceeding this without a terminator found are a
  protocol error (reject the connection or discard, never grow the buffer
  unbounded) — this is the transport bound Person 1 (NET) must enforce: a
  per-client read buffer must stop accumulating and signal an error once it
  passes 512 bytes without producing a complete line, instead of growing
  forever (unbounded-buffer DoS risk).
- **Terminator tolerance:** accept lines ending in `\r\n` **or** bare `\n`.
  Two different real sources justify this: the reference client (irssi, see
  `docs/reference-client.md`) always sends strict `\r\n`; the subject's own
  `nc` test (IV.3) sends `"com"^D"man"^D"d\n"` — terminated by bare `\n` only.
  Requiring strict CRLF would fail the subject's own test case.
- **Parameter limit:** 15 parameters maximum per command (RFC 2812), the last
  of which may be a trailing parameter (starts with `:`, may contain spaces).
- **Casemapping:** adopts the existing `irc::canonicalizeIrcName` helper
  (`include/contracts/IrcCaseMapping.hpp`, proposed in CHAN-02's draft above)
  for nickname comparison — one shared helper for both nickname and channel
  lookup, no independent lowercase policy in the parser.

### Examples

- **Normal CRLF line:** `NICK bob\r\n` — one `recv()` call, complete line.
- **Split across packets:** `PRIVMSG #general :hi` arrives as `"PRIVMSG #gen"`
  in one `recv()` and `"eral :hi\r\n"` in the next — the per-client read
  buffer accumulates both fragments; parsing only triggers once a terminator
  (`\r\n` or `\n`) is found in the accumulated buffer, consuming only the
  completed line and keeping any remainder for the next line.
- **Trailing parameter with spaces:** `PRIVMSG #general :salut les amis` —
  naive splitting on every space would wrongly cut `"salut les amis"` into
  3 separate parameters; the `:` marks the start of a single trailing
  parameter that extends to the end of the line, spaces included.

## Registry operations (CHAN-03)

`ChannelRegistry` owns a `map<string, Channel>` by canonical key. It is
noncopyable and never uses map indexing for lookup. Every operation first
uses the same channel-name validation and shared mapping:

| Operation | Result | Mutation |
| --- | --- | --- |
| `find(name)` (mutable/const overloads) | Pointer to channel; null if absent/invalid | None; never inserts |
| `create(name)` | `(pointer, true)` when inserted; `(existing, false)` for an alias; `(null, false)` if invalid | One insertion at most; existing state and first display spelling stay intact |
| `eraseEmpty(name)` | True only when removed; false if absent/invalid/nonempty | Erases the entire channel value only when its member set is empty |
| `size()` | Current channel count | None |

Pointers remain valid across insertions and erasure of **other** channels;
erasing that channel or destroying its registry invalidates them. Callers
must not retain pointers across those lifetimes. Allocation failures
propagate without a partial insertion; runtime failure containment belongs
to the future server layer. Erasing a value destroys all its owned
topic/modes/invitation state, so recreation receives fresh defaults.
Invitations do not keep an otherwise empty channel alive. Checking
membership for erasure does not implement JOIN/PART or CHAN-04 operations.

## Shared channel boundary (CORE-03)

`include/contracts/ChannelService.hpp` adds the following interfaces without
renaming CORE-02's existing global `protocol`, `transport`, `ClientIdentity`
or `enqueueResult` types:

| Provider | Signature | Meaning |
| --- | --- | --- |
| Person 2: `irc::IdentityLookup` | `bool identityOf(ClientId, ::ClientIdentity &out) const` | Copies a snapshot; false leaves output unchanged |
| Person 2: `irc::IdentityLookup` | `bool clientIdForNickname(const std::string &, ClientId &out) const` | Uses the shared canonical nickname policy; false leaves output unchanged |
| Person 2: `irc::IdentityLookup` | `bool isRegistered(ClientId) const` | False for unknown or incomplete sessions |
| Person 3: `irc::ChannelService` | `void handle(ClientId, const ParsedCommand &)` | Validates before mutation and queues replies via transport |
| Person 3: `irc::ChannelService` | `void removeClient(ClientId)` | Idempotent state cleanup; caller owns notifications |
| Person 3: `irc::ChannelService` | `std::vector<ClientId> peersOf(ClientId) const` | Detached, deduplicated IDs excluding the actor, with no FD/iterator exposure |

The lookup provider is a required integration interface, not an adapter that
pretends the current minimal ClientRegistry already implements identity or
registration. Real nickname lookup/registration and all command handlers
remain Person 2's tasks. The separately compiled fixtures demonstrate only
these signatures, snapshot isolation and a validation-before-mutation example;
their `FIXTURE` command is test-only and is not an IRC feature.

### Ownership and lifecycle ordering

Before releasing a connection: (1) protocol snapshots the actor's identity
and channel peers while both are live, (2) protocol queues NICK/QUIT or other
applicable notifications to the deduplicated recipients, (3) channel cleanup
removes memberships, operators and invitations and deletes empty channels,
(4) protocol releases nickname/identity state, then (5) transport closes and
removes its descriptor/buffers. No channel handler closes descriptors or
calls recv/send. `requestDisconnection(..., afterFlush)` keeps CORE-02's
existing queue-drain semantics; a duplicate cleanup must have no effect.

### Channel policies to approve

- One canonical key uses the existing RFC1459 helper and CHAN-02 validation;
  display spelling is the first creator's spelling, not identity.
- The successful first JOIN grants the creator operator status. A duplicate
  JOIN never grants or restores privilege. Merely adding data-model membership
  does not implement JOIN or automatically grant an operator role.
- A departing/demoted last operator causes **no automatic succession**.
  Existing regular members stay regular; an operatorless channel remains
  until its last member leaves. Recreation starts with fresh state and a
  newly joining creator. This is a project policy, not an extra IRC mode.
- Empty channels are deleted even if topic, modes or invitations remain.
  Removing a client revokes invitations everywhere, including channels the
  client has never joined. Peer notifications happen before removal.
- All required parameter/registration/permission checks precede mutation of
  that operation. Rejected validation changes no topic, role or membership.
- Mixed MODE is processed in order after a parameter-consumption plan is
  built. Known `+k`, `+o`, `-o`, `+l` consume one argument; `-k`, `-l`,
  `+/-i`, `+/-t` consume none. Unknown flags emit 472 and consume none.
  An invalid known operation consumes its assigned argument but changes no
  state; valid independent operations may succeed. Missing required arguments
  emit 461 and stop the remaining operations to avoid reinterpreting tokens.
  These are contract expectations, not an implementation of MODE parsing.

The root README and other owners' implementations remain unchanged.

## Verification

Run `python3 tests/channels/run.py --suite model` from the repository root.
The fixture constructs an empty channel without a Server, mutates only a
test nickname lookup, checks old identities cannot be inherited by another
ID and checks copied channel state is independent. It does not establish
registration, reference-client interoperability or a working IRC server.

Tests use standard-library C++98 only. The Python runner builds in an owned
temporary directory, enforces compile/run timeouts and removes only its own
temporary artifacts. Compiler/test errors produce a nonzero status.
