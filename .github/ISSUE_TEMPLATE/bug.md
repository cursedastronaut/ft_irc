---
name: Bug Report / Fix
about: Report and fix an unexpected behavior or crash in ft_irc
title: "[BUG-XX] Short description of the bug or crash"
labels: ["bug"]
assignees: []
---

## Description
[Describe the unexpected behavior, crash, or memory leak. Define done as a single observable fix, a focused reproduction/verification test case, and no unrelated code rewrites.]

## Owner and size
Person X. Estimated focused work: X.X h, excluding review (max 0.5 day).

## Dependencies
`[TASK-XX]` (or None)

## Acceptance criteria
- [ ] Bug is fixed on owned paths with a single observable behavior.
- [ ] Focused reproduction case included (how to trigger the bug vs. how it passes now).
- [ ] `ircserv` remains strictly C++98, authorized functions only, no external libraries.
- [ ] Reviewer confirms no unrelated behavior or subsystem was altered.

## Collaboration rules
Owner: Person X | Reviewer: Person Y | Shared contracts untouched unless approved by all 3.

## Scope
- [ ] mandatory scope.
- [ ] bonus scope.