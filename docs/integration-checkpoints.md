# Incremental integration checkpoints — CORE-08

A coordination gate over the smaller NET/PROTO/CHAN tasks, not an implementation
epic. At each checkpoint, the named lead demonstrates the current state using
stubs or the real server, with a short runnable smoke test — not a full
implementation of the feature.

| Checkpoint | Smoke test | Lead |
|---|---|---|
| Buildable stubs | `make` compiles with zero warnings on all three machines | Person 1 |
| Connect/read/write | `nc localhost <port>` connects, sends bytes, receives a response | Person 1 |
| PASS/NICK/USER | Reference client (irssi, see `docs/reference-client.md`) completes registration and receives a `001` welcome numeric | Person 2 |
| Private chat | Two connected clients; `PRIVMSG` between them is delivered | Person 2 |
| JOIN/channel chat | `JOIN` + `PRIVMSG` to a channel is seen by all members | Person 3 |
| Operator commands/modes | `KICK`/`INVITE`/`TOPIC`/`MODE` exercised via the reference client | Person 3 |
| Mandatory release | Clean Linux build, no crash, full subject test plan passes | Rotation (whoever merges the final mandatory PR) |
| Optional bonuses | Depends on which OPT issues are picked | TBD once OPT issues are assigned |
