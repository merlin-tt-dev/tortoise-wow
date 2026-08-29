# LivingBots clean port — 0037

Apply after the committed 0036 source state (`a5fc8fb`).

Patch:

`mod/0037-build-compatibility-pass-8-native-travel-graph-mmap-taxi.patch`

Scope: module-only native Penqle travel graph, WorldLocation/GameTele field migration,
MMap tile loading through Penqle `MMapManager::loadMap`, Penqle TaxiPath `Path<>`
indexed access, travel-target RNG/NPC flags, and Group raid naming.

No repository `patches/` bookkeeping is included.
