# Known Sanitizer Findings

## Logger level-state race

TSAN validation of `ClientServerTest` with the Network event loop and worker pool reported a data race in the vendored StormByte Logger, not in Network-owned code.

Observed access pair:

- Read: `StormByte::Logger::Log::Write(const Logger::Level&)` from the server accept/event-loop thread.
- Previous write: `StormByte::Logger::Log::Write(std::ostream& (*)(std::ostream&))` from the main test thread during server connection/logging.
- Allocation origin: the shared `ThreadedLog` instance created during test initialization.

The finding occurred in Logger's internal level/current-message state while Network was logging concurrently. Network currently shares a logger across the event-loop and worker threads, as intended by the existing API. Do not work around this in Network by removing logs, adding arbitrary sleeps, or changing thread ownership.

## Reproduction

This was observed on Linux with GCC 15.3.1, Ninja, bundled third-party dependencies, and the current `ClientServerTest`. Use a fresh temporary build directory; do not reuse the normal user `build/` directory.

Configure and build:

```sh
cmake -S . -B build-agent-tsan-gcc -G Ninja \
	-DCMAKE_BUILD_TYPE=Debug \
	-DCMAKE_C_COMPILER=gcc \
	-DCMAKE_CXX_COMPILER=g++ \
	-DENABLE_TEST=ON \
	-DCMAKE_C_FLAGS='-O1 -g -ggdb3 -fsanitize=thread -fno-omit-frame-pointer' \
	-DCMAKE_CXX_FLAGS='-O1 -g -ggdb3 -fsanitize=thread -fno-omit-frame-pointer' \
	-DCMAKE_EXE_LINKER_FLAGS='-fsanitize=thread' \
	-DCMAKE_SHARED_LINKER_FLAGS='-fsanitize=thread' \
	-DCMAKE_C_COMPILER_LAUNCHER=ccache \
	-DCMAKE_CXX_COMPILER_LAUNCHER=ccache
ninja -C build-agent-tsan-gcc test/ClientServerTest
```

For the captured run, the command sequence returned `TSAN_DEBUG_BUILD:0` and `TSAN_DEBUG_TEST:8`. The test command was run with:

```sh
TSAN_OPTIONS='halt_on_error=1:second_deadlock_stack=1:history_size=7'
```

Run the registered test with TSAN enabled:

```sh
TSAN_OPTIONS='halt_on_error=1:second_deadlock_stack=1:history_size=7' \
	ctest --test-dir build-agent-tsan-gcc/test \
	-R ClientServerTest --output-on-failure
```

The report may appear during the first request test or another early server/client test because it depends on scheduling. The important signature pair is:

```text
Read:    StormByte::Logger::Log::Write(const Logger::Level&)
				 from Server::AcceptClients()/EventLoop thread
Write:   StormByte::Logger::Log::Write(std::ostream& (*)(std::ostream&))
				 from the main test thread during Server::Connect/logging
Object:  shared ThreadedLog implementation allocated during test startup
```

The symbolized TSAN report captured in this repository identified the race as:

```text
WARNING: ThreadSanitizer: data race
Read of size 1 by thread T9:
	StormByte::Logger::Log::Write(const StormByte::Logger::Level&)
		.../StormByte-Logger/src/lib/public/StormByte/logger/log.cxx:63
	StormByte::Logger::ThreadedLog::Write(const StormByte::Logger::Level&)
		.../StormByte-Logger/src/lib/public/StormByte/logger/threaded_log.cxx:241

Previous write of size 1 by main thread:
	StormByte::Logger::Log::Write(const StormByte::Logger::Level&)
		.../StormByte-Logger/src/lib/public/StormByte/logger/log.cxx:63
	StormByte::Logger::ThreadedLog::Write(const StormByte::Logger::Level&)
		.../StormByte-Logger/src/lib/public/StormByte/logger/threaded_log.cxx:241

SUMMARY: ThreadSanitizer: data race
	.../StormByte-Logger/src/lib/private/StormByte/logger/implementation.cxx:424
	in StormByte::Logger::Implementation::operator<<(const Logger::Level&)
```

The captured build also verified DWARF sections in both the vendored Logger shared library and `ClientServerTest`: `.debug_info`, `.debug_abbrev`, `.debug_line`, `.debug_str`, and `.debug_line_str` were present. This means the locations above came from the explicit `-g -ggdb3` build rather than symbol guessing.

The exact source line may be unavailable because the vendored Logger is built without debug symbols in the normal CMake configuration. The symbolized function names identify the ownership: the same `Log` object's mutable current-level/current-line state is accessed concurrently by logger calls from different threads.

If one run does not report the race, repeat the same CTest command a few times without changing code or logger configuration. A clean run is not proof that the race is absent. Do not add sleeps or change the logger level to hide it; those only alter scheduling and can make the report disappear.

After fixing Logger, rebuild Logger and Network from clean temporary directories and rerun this exact TSAN configuration plus the normal GCC/Clang test suites.

Future remediation belongs in StormByte-Logger: make the relevant per-line/current-level state thread-safe, or clearly separate immutable configuration from thread-local message state. Re-run TSAN against Network after the Logger fix.

Status: known external dependency finding; no Network fix applied.
