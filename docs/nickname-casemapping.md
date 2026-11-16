# Nickname casemapping — PROTO-09 (#44)

Reference table for the RFC1459-style casemapping already implemented in
`irc::canonicalizeIrcName` (`include/contracts/IrcCaseMapping.hpp`, shared
CORE-03/PROTO-01 contract). Used by `ClientRegistry::setNickname` /
`clientIdForNickname` to decide when two spellings refer to the same client.

## Transformation rules

| Input            | Canonical form | Rule                              |
|------------------|-----------------|-----------------------------------|
| `A`–`Z`          | `a`–`z`         | ASCII case fold (ordinary letters)|
| `[`              | `{`             | RFC1459 special-char equivalence  |
| `]`              | `}`             | RFC1459 special-char equivalence  |
| `\`              | `\|`            | RFC1459 special-char equivalence  |
| `^`              | `~`             | RFC1459 special-char equivalence  |
| anything else    | unchanged       | digits, `-`, already-lowercase, … |

## Worked examples — equivalent spellings (same key)

| Spelling A   | Spelling B   | Canonical key |
|--------------|--------------|---------------|
| `Bob`        | `BOB`        | `bob`         |
| `Bob[x]`     | `BOB[X]`     | `bob{x}`      |
| `Bob[x]`     | `bob{x}`     | `bob{x}`      |
| `a\b`        | `A\|B`       | `a\|b`        |
| `nick^`      | `NICK~`      | `nick~`       |

## Worked examples — distinct permitted nicknames (stay distinct)

| Nickname A | Nickname B | Canonical keys differ? |
|------------|------------|-------------------------|
| `bob`      | `rob`      | yes — `bob` vs `rob`   |
| `bob`      | `bobby`    | yes — `bob` vs `bobby` |
| `bob{x}`   | `bob{y}`   | yes — `bob{x}` vs `bob{y}` |

## Verification

```sh
python3 tests/contracts/run.py --suite casemapping_contract
```

See `tests/contracts/casemapping_contract.cpp`.
