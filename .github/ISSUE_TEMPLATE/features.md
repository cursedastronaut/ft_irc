---
name: Standard Task / Feature
about: Standard implementation ticket for ft_irc (v11.0)
title: "[TASK-XX] Short description of the feature or module"
labels: ["task"]
assignees: []
---

## Description
[Describe the concrete deliverable or single observable behavior to implement. Define done as one observable behavior, a focused verification, and no unrelated rewrite.]

## Owner and size
Person X. Estimated focused work: X.X h, excluding review (max 0.5 day).

## Dependencies
`[TASK-XX]` (or None)

## Acceptance criteria
- [ ] Implements one observable behavior with no unrelated rewrite.
- [ ] Keeps `ircserv` strictly C++98 with only authorized functions and no external libraries.
- [ ] Includes focused verification/reproduction in the PR (tested normal example + failure case).

## Collaboration rules
Owner: Person X | Reviewer: Person Y | All three people approve shared contracts. Do not rename frozen interfaces.

## Scope
- [ ] mandatory scope.
- [ ] bonus scope.