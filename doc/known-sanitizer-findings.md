# Known Sanitizer Findings

## Logger level-state race

TSAN validation of `ClientServerTest` with the Network event loop and worker pool reported a data race in the vendored StormByte Logger, not in Network-owned code.

Observed access pair:

- Read: `StormByte::Logger::Log::Write(const Logger::Level&)` from the server accept/event-loop thread.
- Previous write: `StormByte::Logger::Log::Write(std::ostream& (*)(std::ostream&))` from the main test thread during server connection/logging.
- Allocation origin: the shared `ThreadedLog` instance created during test initialization.

The finding occurred in Logger's internal level/current-message state while Network was logging concurrently. Network currently shares a logger across the event-loop and worker threads, as intended by the existing API. Do not work around this in Network by removing logs, adding arbitrary sleeps, or changing thread ownership.

Future remediation belongs in StormByte-Logger: make the relevant per-line/current-level state thread-safe, or clearly separate immutable configuration from thread-local message state. Re-run TSAN against Network after the Logger fix.

Status: known external dependency finding; no Network fix applied.
