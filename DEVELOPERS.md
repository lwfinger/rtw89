DEVELOPERS GUIDE — rtw89 driver

Goal
- Make it easy to profile, optimize, and safely change the driver.

Quick build
```bash
make clean
make -j$(nproc)
```

Installing for runtime testing
```bash
sudo make unload || true
sudo make install || sudo insmod ./rtw_8852be.ko || true
# Use depmod/modprobe style as you prefer
```

Basic profiling with `perf`
- Reproduce the workload (STA + start AP, heavy TX/RX)
- On the host, run:
```bash
# Build with debug symbols (kernel build will keep module symbols)
sudo perf record -g -e cpu-clock -a -- sleep 10
# Or target the module by PID while reproducing traffic
sudo perf report --call-graph=dwarf
```
- Look for hot call stacks in `mac80211` and driver `core` paths (tx path, napi, interrupt handling).

Using ftrace / tracepoints
- Enable `ftrace` or `dynamic_trace` to instrument functions:
```bash
# Example: trace rtw89 functions
echo function > /sys/kernel/debug/tracing/current_tracer
echo rtw89_* > /sys/kernel/debug/tracing/set_ftrace_filter
cat /sys/kernel/debug/tracing/trace > trace.out
```
- Use mac80211 tracepoints (see `include/net/mac80211.h`) to correlate driver events.

Lightweight runtime instrumentation
- Add `rtw89_dbg(rtwdev, RTW89_DBG_PERF, "...")` in suspected hot paths (use a new `RTW89_DBG_PERF` flag) to count events without heavy tracing. Aggregate counts instead of printing every packet.
- Use atomic counters or percpu counters for high-rate counters.

Common optimization targets (what to look for)
- TX path: avoid per-packet mallocs; pre-allocate skb fragments / reuse skb->data where possible.
- RX path: minimize skb cloning and copying; use zero-copy if feasible.
- Locking: keep spinlock/ mutex hold times minimal; prefer trylock in non-critical paths.
- NAPI: ensure napi budget and scheduling is efficient; batch skbs to mac80211.
- Workqueues: avoid creating a work per-packet; schedule batched work.
- Avoid repeated small I/O to HW registers; batch register writes where possible.

Code quality & hygiene
- Use `checkpatch.pl` and `sparse` before sending patches.
- Keep public APIs in `core.h` and group related functions.
- Add small unit tests for helpers that are buildable out-of-kernel where possible.
- Keep changes small and documented: add a short comment block explaining rationale for each micro-optimization.

How to propose a change
1. Profile and point to a hot function/stack.
2. Create a branch, implement minimal change with tests or microbenchmark.
3. Add a `DEVELOPERS.md` note and a `TODO` in code pointing to the profiling output.
4. Submit PR with before/after perf numbers.

Make targets (suggested)
- You can add convenience targets to Makefile locally:
```makefile
perf-record:
	perf record -g -e cpu-clock -a -- sleep 10
perf-report:
	perf report --call-graph=dwarf
```

Safety notes
- Kernel-space changes can crash the system. Test under kdump-enabled VM or with serial.
- Keep debugging macros so they can be turned off for production builds.

Next actions I can take for you
- Run a quick static scan to point out obvious hotspots.
- Add a minimal `RTW89_DBG_PERF` flag and a few counters in the TX/RX path to gather rates.
- Add the `perf` Makefile targets for convenience.

Tell me which of the next actions you want me to perform, or paste `perf report`/`dmesg` output and I will target the real hotspots.
