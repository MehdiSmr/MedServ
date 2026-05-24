# 4-Day C++ Chat Server Learning Roadmap (Beej-based)

### Read (Beej)
- Revisit **Ch. 6** patterns
- **Ch. 8**: Common Questions
- **Ch. 9** refs: `recv`, `send`, `accept`, `errno`, `setsockopt`, `close`, `shutdown`

### Build tasks
1. Implement model: **1 accept thread + 1 worker thread per client**.
2. Protect shared chat state with `std::mutex`.
3. Add structured logs: fd, command, bytes in/out, error code.
4. Test failure scenarios:
   - client disconnect (`recv == 0`)
   - timeout (`EAGAIN/EWOULDBLOCK`)
   - malformed command
   - two clients sending concurrently
5. Write a short README:
   - protocol commands
   - architecture
   - known limitations

### Outcome
You have a small but real multi-client chat server architecture and can debug socket issues systematically.

---

## Day 4 — Event Loop with `poll()` + Architecture Comparison

### Read (Beej)
- **Ch. 7.2**: `poll()`
- Revisit **Ch. 7.1**: blocking vs non-blocking behavior
- **Ch. 7.4**: partial `send()` (for write-readiness handling)

### Build tasks
1. Keep the same line protocol (`LIST\n`, `CREATE room\n`, `MSG room hi\n`).
2. Build a tiny **single-threaded `poll()` server**:
   - monitor listening socket for accepts
   - monitor all client sockets for readable/writable events
3. Keep **per-client input buffers** and parse only full newline-delimited commands.
4. Reuse `sendAll()` or implement a small **per-client outgoing queue** for partial writes.
5. Add basic metrics/logging and compare with thread-per-connection model:
   - 1, 10, 50+ clients
   - CPU, memory, latency, implementation complexity

### Outcome
You understand when to choose thread-per-connection vs event-driven I/O, and you have a working prototype of both.

---

## Stretch goal after Day 4
- Write a tiny load-test script (bash/python) to automate client bursts and collect basic timing stats.
