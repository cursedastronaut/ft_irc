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

## CHAN-07 (#35): invitations

`hasInvitation`, `addInvitation` and `revokeInvitation` use stable ClientIds.
Add/revoke return true only when their set changes. `consumeInvitation(id)`
returns false and preserves the invitation until that ID is a member;
the JOIN caller invokes it only after successful admission is committed.
It is never called during validation. This guards the data boundary without
implementing JOIN admission, INVITE permissions or nickname lookup.

`python3 tests/channels/run.py --suite invitations` covers duplicate adds,
an admission that has not committed, one-time successful consumption,
unknown revocation, nickname changes and a different ID reusing a nickname.
Transport's never-reused-ID contract remains responsible for avoiding
identity reuse; a reused socket/nickname does not become an old ID here.

## CHAN-05 (#40): channel-scoped operators

`isOperator(id)` requires membership in this channel. Grant/revoke return
true only for a changed role of a current member. Unknown/nonmember targets
are rejected. Member removal clears its operator entry. If the last operator
leaves or is demoted, no other member is automatically promoted; the channel
may remain operatorless until it becomes empty and is recreated. Successful
first-JOIN creator assignment remains CHAN-10, not a side effect of these
data APIs. Authorization of the acting command issuer remains the later
operator command layer; these methods only guard the target's membership.

`python3 tests/channels/run.py --suite operators` covers both channels,
promotion/demotion, duplicate operations, nonmembers and last-operator exit.

## CHAN-09 (#41): peer snapshots

`ChannelRegistry::peersOf(actorId)` returns a sorted, deduplicated vector of
other member IDs from channels shared with the actor. Invitations, outsider
channels and the actor itself are excluded. The returned collection owns its
values and survives later registry changes. The future ChannelService can
delegate this data operation; the registry does not build or send NICK/QUIT.

`python3 tests/channels/run.py --suite peers` covers two shared channels,
one unrelated channel, an invite-only outsider, an unknown actor and snapshot
isolation after membership changes.

## CHAN-08 (#48): disconnect cleanup

`ChannelRegistry::removeClient(actorId)` drops membership, operator status
and invitations for this ID in every channel, including invitations in
channels the actor never joined. It then erases empty channels, including
any pre-existing empty entries. Repeated cleanup is safe. Erasure uses
C++98 iterator ordering and preserves other live channels/pointers.
It has no transport ownership and emits no notification. The caller first
captures the identity and `peersOf` result, queues departure notifications,
then invokes this data hook before releasing protocol identity/transport.
This hook backs the future concrete ChannelService without adding a dummy
`handle()` or depending on the CORE contract PR to compile.

`python3 tests/channels/run.py --suite cleanup` covers multi-channel fake
disconnect, consecutive empty erasures, invitations without membership,
unrelated state preservation, last-operator exit, repeated/unknown cleanup
and fresh state after recreation. It does not simulate a live socket close.

## Stopping point

CHAN-10 (#49) requires PROTO-12 (#39), Person 2's actual parameter and
registration guards. Those are absent from this base. This delivery stops
before JOIN; no fake production guard or work on later issues is included.
