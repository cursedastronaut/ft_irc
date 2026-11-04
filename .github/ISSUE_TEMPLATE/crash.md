---
name: Crash / Critical Fault Report
about: Report an unhandled exception, segmentation fault, or critical server crash
title: "[CRASH-XX] Short description of the crash or core dump"
labels: ["bug", "crash"]
assignees: []
---

## Description
[Describe the exact scenario that triggered the crash (e.g., specific IRC command sequence, malformed packet, client disconnection). Define done as a single observable fix, a focused reproduction/verification test case, and no unrelated code rewrites.]

## Owner and size
Person X. Estimated focused work: X.X h, excluding review (max 0.5 day).

## Dependencies
`[TASK-XX]` (or None)

## Acceptance criteria
- [ ] Crash root cause identified and fixed on owned paths.
- [ ] Focused reproduction case included (how to trigger the core dump/fault vs. stable behavior now).
- [ ] `ircserv` remains strictly C++98, authorized functions only, and zero external libraries (helper test scripts like Valgrind/GDB may use external tools).
- [ ] Reviewer confirms no unrelated behavior or subsystem was altered.

## Collaboration rules
Owner: Person X | Reviewer: Person Y | Shared contracts untouched unless approved by all 3.

## Scope
- [ ] mandatory scope.
- [ ] bonus scope.