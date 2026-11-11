# Channel reply and permission fixtures (CORE-06)

This is the response-policy proposal for Person 2 and Person 3 review. It
does not implement replies or claim either owner's approval. Merge after
the shared response shapes and policies have been reviewed. Existing
`docs/reference-client.md` selects irssi; no extra RFC features are added.

Reusable actors and both channels are defined in
`tests/fixtures/channel_permissions.json`. IDs 1001..1004 are symbolic
stable connection identities, never file descriptors. In #alpha, 1001 is
the operator, 1002/1003 are regular members and 1004 is outside. In #beta,
1002 is operator and 1001 is regular, testing channel-scoped authority.
Nickname changes do not change any ID or expected recipient list.

## Reply shapes

Every queued numeric has the form `:ircserv NNN <actorNick> <params> :<text>`.
Transport appends CRLF exactly once; channel code never includes it in the
queued line. Identity prefixes for events are derived from protocol snapshots,
`:<nick>!<user>@<host>`, never a client-supplied prefix. The actor receives
errors alone; denied operations have no success event or state mutation.

| Numeric | Parameters after actor nickname | Meaning |
| --- | --- | --- |
| 401 | `<target>` | Unknown nickname |
| 403 | `<channel>` | Unknown channel |
| 404 | `<channel>` | Channel send denied (membership-only sending) |
| 441 | `<target> <channel>` | Target is not a channel member |
| 442 | `<channel>` | Actor is not a channel member |
| 443 | `<target> <channel>` | Target is already a member |
| 451 | none | Actor is not registered |
| 461 | `<command>` | Missing required parameters |
| 471 | `<channel>` | Channel user limit reached |
| 472 | `<flag>` | Unknown mode flag |
| 473 | `<channel>` | Invite-only admission refused |
| 475 | `<channel>` | Missing or incorrect channel key |
| 476 | `<channel>` | Invalid channel name |
| 482 | `<channel>` | Channel operator privilege required |
| 324 | `<channel> <flags> [parameters]` | Current modes, canonical order i,t,k,l |
| 331 | `<channel>` | No topic |
| 332 | `<channel>` | Current topic in trailing text |
| 341 | `<target> <channel>` | Invitation acknowledgement |
| 353 | `= <channel>` | Names in trailing text, @ before operators |
| 366 | `<channel>` | End of names |

Human-readable trailing text is a nonempty explanation, except topic events
may carry explicitly empty trailing text for clearing. Numeric errors never
echo server/channel passwords. A 324 query is member-only: operators may
see the channel key; regular members receive `*` in its parameter position.
Presence of +k is visible, but the key itself is not. The server PASS secret
is never a channel key. Reply splitting must later respect PROTO-01 limits.

| Operation | Allowed event / recipients | Denied state / reply |
| --- | --- | --- |
| JOIN | `:<prefix> JOIN :<channel>` to all members after admission | No admission; 473/475/471 as applicable |
| PART | `:<prefix> PART <channel> [:<reason>]` to pre-departure members | No removal; 403 or 442 |
| PRIVMSG | `:<prefix> PRIVMSG <channel> :<text>` to every other member | No delivery; 404 for outsider |
| KICK | `:<prefix> KICK <channel> <target> :<reason>` to pre-removal members, including target | No removal; 482/441 |
| INVITE | `:<prefix> INVITE <target> :<channel>` to target; 341 to actor | No invitation; 482 under +i, 401/443 for invalid target |
| TOPIC query | 331/332 to requesting member only | No mutation; 403/442 |
| TOPIC change | `:<prefix> TOPIC <channel> :<text>` to all members | Existing topic retained; 482 under +t for regular member |
| MODE change | `:<prefix> MODE <channel> <accepted flags> [arguments]` to all members | Rejected operation unchanged; 482/461/472 |

Outside +i, any channel member may invite; with +i, only its operators may
invite. Topic edits are member-only under -t and operator-only under +t.
All i/t/k/o/l changes are channel-operator-only. TOPIC view is member-only.
No ban modes, global administrators or custom client behavior is introduced.
The JSON contains allowed and denied examples for every operation and both
signs of all five required modes, including a cross-channel role denial.
`event_recipients` excludes numeric-only replies; `numeric` errors/replies
go to the actor, and an INVITE success additionally has `ack_numeric` 341.

## Combined mode expectations

Use the consumption/error policy in `docs/team-contract.md`. For example,
`+it-k+ol MemberTwo 4` consumes no argument for i/t/-k, then one each for
+o and +l. `+o+l` with no arguments reports 461 and applies neither. An
unknown flag in `+x+i` reports 472 while independent +i can still succeed.
These are declarative fixtures, not a working MODE parser or handler test.

## Verification

```sh
python3 -B -m unittest discover -s tests/fixtures -p 'test_*.py' -v
```

Integrity checks reject missing deny cases, invalid operator identity,
duplicate/leaked recipients and denied mutations. Actual reply formatting,
registration guards and live permission behavior remain PROTO/CHAN tasks;
a passing specification check must not be reported as a successful live
client or handler test.
