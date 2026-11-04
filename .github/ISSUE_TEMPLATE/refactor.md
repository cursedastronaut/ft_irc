---
name: Refactor Task
about: Clean up or restructure existing code without changing external behavior
title: "[REFACTOR-XX] Short description of code cleanup"
labels: ["refactor"]
assignees: []
---

## Description
[Describe the code structure to improve, why it needs refactoring, and how it aligns with ft_irc v11.0 standards. Remember: definition of done means one observable behavior/goal, focused verification, and no unrelated rewrites.]

## Owner and size
Person X. Estimated focused work: X.X h, excluding review. Must remain under half a day; split if larger.

## Dependencies
`[TASK-XX]` (or None)

## Acceptance criteria
- [ ] Code is cleaned/restructured on owned paths without altering external/observable behavior.
- [ ] `ircserv` remains strictly C++98, authorized functions only, and zero external libraries.
- [ ] Focused verification included to prove behavior remains identical (normal case + failure case).
- [ ] No frozen interfaces renamed and no unrelated subsystems touched.

## Collaboration rules
Owner: Person X | Reviewer: Person Y | Shared contracts approved by all 3 if modified.

## Scope
- [ ] mandatory scope.
- [ ] bonus scope.