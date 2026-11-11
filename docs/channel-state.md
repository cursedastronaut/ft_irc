# Channel state operations

These are server-independent Person 3 data operations, not JOIN handlers,
registration guards or socket code. Existing public value fields remain
available for compatibility; services should use the guarded operations.
Review against the CORE-03 policy before merging shared consumers.

## CHAN-04 (#33): membership

`containsMember(id)` is a const lookup. `addMember(id)` returns true only
for a new member. `removeMember(id)` returns true only if that member was
present and also removes that ID's operator entry. Other clients and
channels are untouched. It does not consume/revoke invitations or perform
notifications. A raw membership insertion does not grant operator status:
the future successful first-JOIN path is responsible for creator promotion.

Normal/error verification: `python3 tests/channels/run.py --suite membership`.
Duplicate adds and absent removals return false; removing a member clears
only its role. The fixture includes another member/channel and an invitation
to catch unintended cleanup. Existing CI runs every registered suite.

## CHAN-06 (#34): topic and mode storage

The merged CHAN-01 model already contains `topic` and a separate
`ChannelModes` value. No duplicate fields or replacement API are introduced.
This task supplies focused verification of that existing implementation:
topic/key strings start empty, invite-only/topic-restricted/key-present/
limit-present flags start false and absent limit storage is zero. `hasKey`
and `hasLimit` distinguish absence from their parameter storage. These are
value defaults, not a MODE handler or permission implementation.

`python3 tests/channels/run.py --suite state` verifies every default, copy
independence, rejection without resetting state, empty-channel recreation
and channel/server key separation using the actual public ServerConfig API.
The server password never appears in output and is unaffected by channel
state edits. The fixture does not change Person 1's configuration code.
