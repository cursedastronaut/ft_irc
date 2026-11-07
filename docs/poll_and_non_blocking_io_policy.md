# Network I/O & `poll()` Policy (`ft_irc` Subject)

## 1. Scope & Dependencies
- **Target Specification:** `ft_irc` subject mandatory networking foundation.
- **Prerequisites:** `[CORE-01]`, `[CORE-02]`.
- **Out of Scope:** Full-RFC implementation, custom IRC client, or server-to-server networking. Frozen interfaces and unrelated subsystems remain untouched.

---

## 2. Core Event Loop & Readiness Policy

1. **Single `poll()` Call Site:**
   - The server architecture enforces **exactly one** `poll()` call site across the entire codebase to monitor listening sockets, client reads (`POLLIN`), and client writes (`POLLOUT`).
2. **Strict Readiness Requirement:**
   - Every `recv()` or `send()` attempt requires explicit readiness reported by `poll()` (`revents`) during the **current** poll cycle.
   - No socket read or write may be triggered outside the central `poll()` dispatch loop.
3. **Partial Operations & No Blind Draining:**
   - At most **one** `recv()` and **one** `send()` may be executed per ready file descriptor per `poll()` cycle.
   - After a partial read or a partial write, execution **must return to `poll()`** on the next cycle rather than looping to drain or flush the socket blindly.

---

## 3. Return-Value-Only Handling Contract (No `errno`)

Inspecting `errno` after `recv()` or `send()` is **strictly forbidden** (even to check for `EAGAIN` or `EWOULDBLOCK`). Branching relies exclusively on the syscall return value:

| Syscall | Return Value | Mandatory Action |
| :--- | :--- | :--- |
| `recv()` | `> 0` | **Append** the received bytes to that client's input buffer (`in_buffer`), parse any complete `\r\n`-terminated messages, and return to `poll()`. |
| `recv()` | `== 0` | **Disconnect** the client cleanly (peer closed connection) and remove its descriptor from `poll_fds`. |
| `recv()` | `< 0` | **Close** that client immediately (fatal read failure; do not inspect `errno`). |
| `send()` | `> 0` | **Advance** (erase) the client's output buffer (`out_buffer`) by the exact number of bytes sent and return to `poll()`. |
| `send()` | `<= 0` | **Close** that client immediately (fatal write failure; do not inspect `errno`). |

---

## 4. Non-Blocking Mode & Subject Restrictions

- **Non-Blocking Sockets:** Every socket (listening socket and every accepted client socket) must be configured in non-blocking mode immediately upon creation/acceptance.
- **Subject `fcntl` Rule:** Only the exact call below is permitted to set non-blocking mode:
  ```cpp
  fcntl(fd, F_SETFL, O_NONBLOCK);
  ```
- **Prohibited Dependencies & Calls:**
  - No `fork()` or multiprocessing.
  - No Boost or external networking/implementation libraries.

---

## 5. Reference Implementation Pattern

```cpp
// 1. Single poll() call site for listening, client reads, and client writes
int ready = poll(&poll_fds[0], poll_fds.size(), timeout);

for (size_t i = 0; i < poll_fds.size(); ++i) {
    // Case A: Listening socket is ready -> accept incoming client connection
    if (poll_fds[i].fd == listen_fd && (poll_fds[i].revents & POLLIN)) {
        int client_fd = accept(listen_fd, NULL, NULL);
        if (client_fd >= 0) {
            // Apply non-blocking mode using the only allowed fcntl signature
            fcntl(client_fd, F_SETFL, O_NONBLOCK);
            // Register client_fd in poll_fds
        }
    }
    // Case B: Client socket is ready for reading (at most one recv() per poll cycle)
    else {
        if (poll_fds[i].revents & POLLIN) {
            ssize_t n = recv(poll_fds[i].fd, buf, sizeof(buf), 0);
            if (n > 0) {
                // Append received bytes to client buffer and return to poll() on next cycle
                clients[poll_fds[i].fd].in_buffer.append(buf, n);
            } else if (n == 0) {
                // Client disconnected cleanly (never inspect errno)
                disconnect_client(poll_fds[i].fd);
                continue;
            } else {
                // Read error (n < 0): close client immediately without checking errno/EAGAIN
                close_client(poll_fds[i].fd);
                continue;
            }
        }
        // Case C: Client socket is ready for writing (at most one send() per poll cycle)
        if ((poll_fds[i].revents & POLLOUT) && !clients[poll_fds[i].fd].out_buffer.empty()) {
            std::string& out = clients[poll_fds[i].fd].out_buffer;
            ssize_t n = send(poll_fds[i].fd, out.data(), out.size(), 0);
            if (n > 0) {
                // Advance output buffer by n bytes; return to poll() if partial send occurred
                out.erase(0, n);
            } else {
                // Write error or zero return (n <= 0): close client without checking errno
                close_client(poll_fds[i].fd);
            }
        }
    }
}
```

---

## 6. Permitted Syscall & Compliance Checklist

- [x] Exactly **one** `poll()` call site in the entire project.
- [x] Every socket configured via `fcntl(fd, F_SETFL, O_NONBLOCK)`.
- [x] Zero occurrences of `errno`, `EAGAIN`, or `EWOULDBLOCK` after `recv()` / `send()`.
- [x] Zero `while` loops draining `recv()` or flushing `send()` within a single `poll()` cycle.
- [x] Zero uses of `fork()`, Boost, or external libraries.

---

## 7. Focused Verification & Reproduction Steps

Run the following checks to verify compliance during PR review:

**Static Policy Verification (Syscalls & Restrictions):**
   ```bash
   # Verify exactly one poll() call site exists
   grep -rn "poll(" src/ include/

   # Verify only fcntl(fd, F_SETFL, O_NONBLOCK) is used
   grep -rn "fcntl" src/ include/

   # Confirm absence of errno branching and fork()
   grep -rn "errno\|EAGAIN\|EWOULDBLOCK\|fork" src/ include/
   ```

---

## 8. Shared Contract Sign-Off

- [x] **Person 1 (Owner):** Authored policy and verification steps.
- [ ] **Person 2 (Reviewer):** Verified compliance with subject and confirmed no unrelated behavior changed.
- [ ] **Person 3:** Reviewed and approved shared I/O contract.