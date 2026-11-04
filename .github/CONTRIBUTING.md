# Contributing Guidelines — ft_irc (v11.0)

## 1. Project Scope & Technical Constraints
- **Mandatory Scope Only:** Work is strictly limited to mandatory deliveries and supporting verification/documentation for `ft_irc` subject v11.0. Do not implement full-RFC features, a custom IRC client, or server-to-server networking.
- **C++98 & External Tools Policy:**
  - The submitted `ircserv` binary must strictly remain **C++98**, using **only subject-authorized functions** and **zero external libraries**.
  - Helper test scripts and reproduction harnesses may freely use external tools (e.g., Bash, Python, `nc`, `valgrind`) to automate verification.

## 2. Definition of Done
Every ticket and Pull Request must meet three strict criteria to be considered **Done**:
1. **One observable behavior:** Delivers a single, well-defined behavior (or a dedicated test/documentation patch).
2. **Focused verification:** Includes exact reproduction steps covering both a normal tested example and a failure/error case.
3. **No unrelated rewrite:** Modifies only the necessary owned paths without refactoring unrelated code.

## 3. Ticket Sizing & Dependencies
- **Time Estimates:** Estimates represent focused working time **excluding code review**.
- **Half-Day Rule:** Any implementation ticket estimated at more than **half a day (0.5 day)** must be split into smaller sub-issues before implementation begins.
- **Task IDs & Stubs:** Internal Task IDs (e.g., `[CORE-01]`, `[CORE-04]`) are distinct from GitHub issue numbers. Always link imported issues using these IDs. Start coding only when prerequisites are merged, or rely on team-approved stubs if your task does not require their full implementation.

## 4. Ownership & Collaboration Rules
- **Shared Contracts:** All 3 team members must approve shared interfaces and contracts before they are merged or modified.
- **Subsystem Boundaries:** Only the named ticket owner writes the change within their owned paths. Never rename frozen interfaces or rewrite another member's subsystem inside an implementation ticket.
- **Pull Request Workflow:**
  - Fill out all sections of `.github/pull_request_template.md` (works for both small code patches and test-only patches).
  - The assigned **Reviewer** must run the focused verification and explicitly confirm that no unrelated behavior changed before approving.