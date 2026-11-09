# Reference IRC client — CORE-05

## Client and version
- **Client:** irssi 1.4.5 (20231003 1405)
- **Install:** `brew install irssi` (macOS) / `apt install irssi` (Linux, incl. WSL on Windows)
- **Chosen for:** available as a native-ish build on all three teammates' platforms
  (Linux, macOS via Homebrew, Windows via WSL), minimal/scriptable, widely used for
  ft_irc testing in the 42 community.

## Connection settings (shared, to save identically on all 3 machines)
Agreed test-only values for local dev sessions between the three of us — not a real
secret, just a fixed value so everyone's client config matches:
- **Port:** `6667`
- **Password:** `ft_irc_test`

```
/set nick ReviewerBot
/connect localhost 6667 ft_irc_test
```
(swap `localhost` for the host running `ircserv` if testing across machines on the
same network; port/password stay the same)

## Captured expected handshake
Captured by putting a minimal stub server (just enough to answer `CAP LS` so irssi
doesn't hang) in place of a real ft_irc server, and recording bytes received, since
PROTO-01 doesn't exist yet. Raw lines, in order, verbatim (CRLF-terminated):
```
CAP LS 302
JOIN :
CAP END
PASS ft_irc_test
NICK ReviewerBot
USER nehuen nehuen localhost :Nehuen
```

**Observations:**
- `CAP LS 302` / `CAP END`: IRCv3 capability negotiation. **Not in ft_irc's mandatory
  scope.** The server must not hang or crash on an unrecognized `CAP` — either ignore
  it silently or reply minimally; irssi proceeds with registration either way once it
  gets *any* response to `CAP LS` (or possibly times out and proceeds — to verify).
- `JOIN :` is sent **before** `PASS`/`NICK`/`USER` complete, every time (reproduced
  twice). The server cannot assume commands arrive strictly in registration order —
  it must check registration state per-command and reject/ignore pre-registration
  commands other than PASS/NICK/USER/CAP/QUIT (e.g. with `451 ERR_NOTREGISTERED`),
  not just process whatever arrives first.
- `PASS`/`NICK`/`USER` match the subject's expected registration sequence.
- `USER` second/third fields (`nehuen localhost`) are whatever irssi fills from the
  local system (not meaningful here); the subject doesn't require validating them.

## Compatibility checklist (mandatory vs optional) — to verify once PROTO-01/CHAN exist
- [ ] **Registration** (mandatory): PASS → NICK → USER accepted in any arrival order
      relative to late/early extra commands; `451` for commands before registration.
- [ ] **Keepalive** (verify against subject): server/client PING↔PONG round-trip
      doesn't drop the connection.
- [ ] **Member list** (mandatory, JOIN-related): `JOIN`, `NAMES`/`353`/`366` show the
      right members.
- [ ] **Topic** (mandatory): `TOPIC` get/set, `331`/`332` numerics.
- [ ] **Messaging** (mandatory): `PRIVMSG` to a channel and to a user.
- [ ] **Not mandatory, must not break anything**: `CAP`, and any other command irssi
      sends by default that isn't in the subject's command list — ignored/rejected
      cleanly, never a crash or hang.
