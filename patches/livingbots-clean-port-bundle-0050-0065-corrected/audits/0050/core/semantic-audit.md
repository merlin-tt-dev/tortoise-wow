# Semantic audit — 0050 core

## LoginQueryHolder core exposure

- Moves the existing `LoginQueryHolder` type out of the private `CharacterHandler.cpp` implementation into `src/game/Handlers/LoginQueryHolder.h`.
- `CharacterHandler.cpp` includes and continues to use the same class implementation.
- No login behavior or query semantics are changed; this is a core API exposure required by the Playerbot module.
- This patch contains **core files only**.
