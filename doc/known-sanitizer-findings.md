# Known Sanitizer Findings

## TSAN: race in StormByte-Logger shared state

### Classification

- **Finding:** data race reported by ThreadSanitizer.
- **Component owning the raced memory:** vendored `StormByte-Logger`.
- **Network involvement:** Network creates and shares the logger across the main thread, the EventLoop thread, and worker-pool threads. Network is the concurrent caller, but the raced mutable state belongs to Logger.
- **Network fix:** none. Do not hide this race in Network by removing logs, adding sleeps, changing scheduling, or changing logger ownership.
- **Future owner:** `StormByte-Logger`.
- **Status:** known external dependency finding, reproduced with full DWARF symbols.

### Exact observed report

The captured TSAN process was `pid=717564`. The first report began with:

```text
==================
WARNING: ThreadSanitizer: data race (pid=717564)
  Read of size 1 at 0x7240000000e1 by thread T9:
```

The first access was symbolized as follows:

```text
#0  std::_Optional_payload_base<StormByte::Logger::Color>::_M_reset()
    /usr/lib/gcc/x86_64-pc-linux-gnu/15/include/g++-v15/optional:337
#1  std::_Optional_base<StormByte::Logger::Color, true, true>::_M_reset()
    /usr/lib/gcc/x86_64-pc-linux-gnu/15/include/g++-v15/optional:560
#2  std::optional<StormByte::Logger::Color>::reset()
    /usr/lib/gcc/x86_64-pc-linux-gnu/15/include/g++-v15/optional:1402
#3  StormByte::Logger::Implementation::operator<<(StormByte::Logger::Level const&)
    thirdparty/StormByte/buffer/src/thirdparty/StormByte/logger/src/lib/private/StormByte/logger/implementation.cxx:424
#4  std::shared_ptr<StormByte::Logger::Implementation>&
    StormByte::Logger::operator<< <std::shared_ptr<StormByte::Logger::Implementation>>(...)
    thirdparty/StormByte/buffer/src/thirdparty/StormByte/logger/src/lib/private/StormByte/logger/implementation.hxx:623
#5  StormByte::Logger::Log::Write(StormByte::Logger::Level const&)
    thirdparty/StormByte/buffer/src/thirdparty/StormByte/logger/src/lib/public/StormByte/logger/log.cxx:63
#6  StormByte::Logger::ThreadedLog::Write(StormByte::Logger::Level const&)
    thirdparty/StormByte/buffer/src/thirdparty/StormByte/logger/src/lib/public/StormByte/logger/threaded_log.cxx:241
#7  StormByte::Logger::Log::operator<<(StormByte::Logger::Level const&)
    build-agent-tsan-gcc-debug/thirdparty/buildmaster/install/include/StormByte/logger/log.hxx:273
#8  std::shared_ptr<StormByte::Logger::Log>&
    StormByte::Logger::operator<< <std::shared_ptr<StormByte::Logger::Log>>(...)
    build-agent-tsan-gcc-debug/thirdparty/buildmaster/install/include/StormByte/logger/log.hxx:485
#9  StormByte::Network::Server::AcceptClients()
    lib/public/StormByte/network/server.cxx:350
#10 std::__invoke_impl<void, void (StormByte::Network::Server::*)() noexcept, StormByte::Network::Server*>(...)
    /usr/lib/gcc/x86_64-pc-linux-gnu/15/include/g++-v15/bits/invoke.h:76
#11 std::__invoke<void (StormByte::Network::Server::*)() noexcept, StormByte::Network::Server*>(...)
    /usr/lib/gcc/x86_64-pc-linux-gnu/15/include/g++-v15/bits/invoke.h:98
#12 std::thread::_Invoker<...>::_M_invoke<0ul, 1ul>(...)
    /usr/lib/gcc/x86_64-pc-linux-gnu/15/include/g++-v15/bits/std_thread.h:303
#13 std::thread::_Invoker<...>::operator()()
    /usr/lib/gcc/x86_64-pc-linux-gnu/15/include/g++-v15/bits/std_thread.h:310
#14 std::thread::_State_impl<std::thread::_Invoker<...>>::_M_run()
    /usr/lib/gcc/x86_64-pc-linux-gnu/15/include/g++-v15/bits/std_thread.h:255
#15 <null> (libstdc++.so.6+0x122c52)
```

The conflicting previous access began with:

```text
  Previous write of size 1 at 0x7240000000e1 by main thread:
    #0 std::_Optional_payload_base<StormByte::Logger::Color>::_M_reset()
       /usr/lib/gcc/x86_64-pc-linux-gnu/15/include/g++-v15/optional:340
```

The remainder of that previous-write stack enters the same Logger level-writing path:

```text
StormByte::Logger::Implementation::operator<<(StormByte::Logger::Level const&)
thirdparty/StormByte/buffer/src/thirdparty/StormByte/logger/src/lib/private/StormByte/logger/implementation.cxx:424

StormByte::Logger::Log::Write(StormByte::Logger::Level const&)
thirdparty/StormByte/buffer/src/thirdparty/StormByte/logger/src/lib/public/StormByte/logger/log.cxx:63

StormByte::Logger::ThreadedLog::Write(StormByte::Logger::Level const&)
thirdparty/StormByte/buffer/src/thirdparty/StormByte/logger/src/lib/public/StormByte/logger/threaded_log.cxx:241
```

The report summary was:

```text
SUMMARY: ThreadSanitizer: data race
  thirdparty/StormByte/buffer/src/thirdparty/StormByte/logger/src/lib/private/StormByte/logger/implementation.cxx:424
  in StormByte::Logger::Implementation::operator<<(StormByte::Logger::Level const&)
```

### Raced object and meaning

TSAN reported the same byte address in both accesses:

```text
0x7240000000e1
```

The object was a heap block of size 248 bytes allocated during logger construction. The relevant object is the shared `ThreadedLog`/`Log::Implementation` state, specifically the optional color/current-line state manipulated while a `Logger::Level` is written.

The important point is not that `std::optional<Color>::reset()` is inherently unsafe. The problem is that the same Logger implementation state is being reset/read or otherwise mutated concurrently by different threads without a TSAN-visible synchronization edge.

The Network call path that exposed the race is:

```text
main test thread
  -> Server::Connect(...)
  -> shared logger writes during startup

EventLoop thread
  -> Server::AcceptClients()
  -> m_logger << Logger::Level::LowLevel << ...
  -> Logger::ThreadedLog::Write(Level)
  -> Logger::Log::Write(Level)
  -> Logger::Implementation::operator<<(Level)
  -> optional<Color>::reset()
```

The Network architecture intentionally shares the logger between these threads. The logger type is named `ThreadedLog`, so this finding should be treated as a Logger thread-safety defect or an incomplete separation of thread-local message state, not as permission to remove concurrency from Network.

### Exact symbol/build evidence

The reproduction used explicit debug flags in the Network and vendored dependency build:

```text
-O1 -g -ggdb3 -fsanitize=thread -fno-omit-frame-pointer
```

The captured report was from GCC 15.3.1 on Linux, with the vendored Logger built as part of the same build. The reported library build ID was:

```text
libStormByte-Logger.so.1.1.0
BuildId: c18a15289a3ab34b72462804bc15c329e28e2bf9
```

The Network library build ID in the stack was:

```text
libStormByte-Network.so.1.0.1.9999
BuildId: 097a167ef1a4f10d5a73879edbb085f0cc098ce5
```

The symbolized Logger library and test executable contained DWARF sections:

```text
.debug_info
.debug_abbrev
.debug_line
.debug_str
.debug_line_str
```

Therefore the file and line locations above are from actual debug information, not guessed symbol offsets.

### Why reproduction may vary

The race is scheduling-dependent. It may appear during the first request, during server startup, or during another early client/server test. A clean run does not prove the race is absent. The relevant interleaving is:

1. The main test thread performs a logger write while connecting/starting a server.
2. The EventLoop/accept thread starts or continues logging through the same shared `ThreadedLog` implementation.
3. Both paths touch the mutable current level/color/line state.
4. TSAN observes an unsynchronized conflicting access.

Changing logger levels, adding sleeps, serializing tests, removing Network log lines, or changing thread timing can make the report disappear without fixing the race.

### What Logger should inspect

StormByte-Logger should inspect at least:

- `implementation.cxx:424`, `Implementation::operator<<(Level const&)`;
- `implementation.hxx:623`, the `shared_ptr<Implementation>` forwarding operator;
- `log.cxx:63`, `Log::Write(Level const&)`;
- `threaded_log.cxx:241`, `ThreadedLog::Write(Level const&)`;
- the mutable `std::optional<Logger::Color>` and any current-level/current-line fields around those methods;
- whether the `ThreadedLog` mutex protects only output serialization or also all mutable per-message state;
- whether message state is truly thread-local, or is currently shared through `Implementation`.

The likely design choices are:

1. Protect all mutable shared message/level/color state with the Logger synchronization primitive; or
2. Keep immutable logger configuration shared and move current-line/current-level state to thread-local storage, with a clear flush protocol.

Do not fix this by making Network stop sharing the logger. That would change the intended Logger contract and hide the defect from other multithreaded consumers.

### Future verification

After fixing StormByte-Logger, rebuild Logger and Network with the same debug-symbol TSAN configuration and verify that:

- the report at `implementation.cxx:424` disappears;
- no replacement race appears in Logger's line flush/output path;
- `ClientServerTest` still passes normally;
- the Network EventLoop, WorkerPool, completion queue, and shutdown tests remain race-free.

**Status:** reproduced and documented; external Logger fix required; no Network workaround applied.
