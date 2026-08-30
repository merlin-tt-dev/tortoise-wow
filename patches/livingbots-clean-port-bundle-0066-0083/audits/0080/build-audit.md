# Build audit 0080

- Environment: ACE Debian 8.0.2; Ninja; MODULE_MOD_PLAYERBOTS=static; USE_PCH=OFF; USE_SCRIPTS=ON; USE_EXTRACTORS=OFF.
- Direct C++ TUs: 4.
- Syntax validation: PASS for all directly affected TUs.
- Object validation: O0/g0 serial/targeted object checks were used. The first combined multi-TU object batch hit the outer runtime limit; heavy TUs were then checked individually and no compiler diagnostics remained. This is recorded as syntax PASS + targeted object PASS rather than claiming one uninterrupted batch.
