# Core optimization series – initial candidates

Audited against `tortoise-wow-dev-clean.tar(4).xz`.

## 001 – ChannelBroadcaster wait/notify

Replaces active spin/poll synchronization in `ChannelBroadcaster` with a
`std::condition_variable`-based state wait.

Key effects:
- no `sleep_for(0ms)` busy-wait in Enable/Disable;
- no tight empty-queue spin while sending is enabled;
- enqueue wakes a sleeping broadcaster immediately;
- stop explicitly wakes and terminates the worker;
- keeps the existing five-message processing batch and channel delivery path.

Files:
- `src/game/Handlers/ChannelBroadcaster.cpp`
- `src/game/Handlers/ChannelBroadcaster.h`

## 002 – WorldTimer steady clock

Replaces the ACE `gettimeofday()` based process-relative timer with
`std::chrono::steady_clock` while preserving the existing millisecond return
type and legacy 32-bit wrap behavior.

Key effects:
- elapsed-time measurements can no longer jump because wall-clock time changes;
- removes the unused `savetime` argument from the private timer helper;
- removes an ACE system-time dependency from `Timer.h`;
- all existing WorldTimer callers remain unchanged.

Files:
- `src/shared/Timer.h`
- `src/shared/Util.cpp`

## Validation

Both patches were checked independently and sequentially with:

    git apply --check --whitespace=error-all <patch>

The patches are intentionally separate because they address unrelated runtime
mechanisms and should be benchmarked/reviewed independently.
