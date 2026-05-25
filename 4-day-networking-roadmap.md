# 4-Day C++ Chat Server Learning Roadmap (Beej-based)

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
# add error handling
# write a readme to explain everything you did and learned and what is the diff between the two implementations
# write a tiny load-test script (bash/python) to automate client bursts and collect basic timing stats.
