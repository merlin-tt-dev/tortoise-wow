# 0040 build audit

Changed C++ translation units:

1. `playerbot/WorldSquare.cpp`
2. `strategy/actions/ChatShortcutActions.cpp`
3. `strategy/actions/GossipHelloAction.cpp`
4. `strategy/actions/QuestAction.cpp`
5. `strategy/actions/RewardAction.cpp`
6. `strategy/actions/SayAction.cpp`
7. `strategy/actions/WorldBuffTravelActions.cpp`
8. `strategy/values/CcTargetValue.cpp`
9. `strategy/values/FreeMoveValues.cpp`
10. `strategy/values/Stances.cpp`

Result: all ten PASS both in the existing audit build and in a separately configured fresh patched tree.

Fresh audit profile:

- Debian ACE 8.0.2
- Ninja
- `MODULE_MOD_PLAYERBOTS=static`
- `USE_PCH=OFF`
- `USE_SCRIPTS=ON`
- `USE_EXTRACTORS=OFF`
- compile validation profile `-O0 -g0`
- serial `-j1`
