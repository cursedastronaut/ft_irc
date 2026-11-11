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
