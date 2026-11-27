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
/set nick RevBot
/connect localhost 6667 ft_irc_test
```
(nick fixé à `RevBot` le 2026-11-27, PROTO-33 — voir "Compatibility checklist"
ci-dessous : `ReviewerBot` dépasse la limite RFC de 9 caractères que respecte
correctement le serveur, et irssi **abandonne la connexion** sur le `432`
plutôt que de redemander un pseudo. Si tu avais `ReviewerBot` dans ta config
irssi existante, `/set nick RevBot` avant de te reconnecter.)
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

## Compatibility checklist (mandatory vs optional) — verified 2026-11-27 (PROTO-32/33)

Verified against the real `ircserv` binary (built from `test`) and real irssi
1.4.5, via a logging TCP proxy between irssi and the server plus a second raw
socket peer (method in `docs/toolbox-proto32-33.md`), not just the original
stub-server capture above.

- [x] **Registration**: PASS → NICK → USER accepted; `001` welcome sent.
      **Gap found and fixed (doc only)**: the nick `ReviewerBot` (11 chars)
      exceeds the server's 9-char RFC nickname limit (`NickHandler.cpp`,
      `isValidNickname`, correct per RFC 2812 grammar — not a server bug).
      irssi receives `432 Erroneous nickname` and **disconnects outright**
      instead of prompting for another nick. Nobody had hit this before:
      the original capture used a dumb stub that never validated anything.
      Fixed by changing the documented nick to `RevBot` above — no protocol
      code change needed, the validation itself is correct.
- [x] **Keepalive**: real irssi PING↔PONG round-trip confirmed on the wire
      (`PING ircserv` → `PONG ircserv :ircserv`), connection stays open.
- [x] **Member list**: `JOIN`, `353`/`366` deliver the right members to a
      real client.
- [x] **Topic**: `TOPIC` set with spaces in the text round-trips exactly
      (`332`/broadcast), confirmed against a real client.
- [x] **Messaging**: channel `PRIVMSG` delivers correctly end-to-end to a
      real client, exact text with spaces preserved. Private `PRIVMSG` to a
      nickname still replies `421 Unknown command` on `test` — **not a new
      gap**, it's `ChannelMessage.cpp`'s known, commented placeholder
      ("until PROTO-31 routes them to Person 2's nickname handler"); already
      fixed in PR #208 (PROTO-28-31), pending Person 1's review.
- [x] **CAP**: covered by PROTO-32 (unit tests in `dispatch_protocol.cpp`);
      confirmed again here on the wire against real irssi.
- Observed, not a PROTO-33 gap: irssi auto-sends `MODE RevBot +i` right after
  registration (trying to set itself invisible). The server replies
  `403 No such channel` (treats the first MODE argument as a channel name
  unconditionally). Harmless — irssi ignores the error and proceeds — and
  user modes aren't in the subject's mandatory scope, so **no fix added**.
- Observed, not a PROTO-33 gap (CHAN territory, flag for Person 3 if
  relevant): after the first `PeerUser` connection dropped and a second one
  with the same nick rejoined, `NAMES` showed neither member with an `@`
  operator prefix, even though one of them should hold ops.
