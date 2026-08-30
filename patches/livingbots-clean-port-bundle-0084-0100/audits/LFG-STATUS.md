# Turtle LFG status at 0100

Active MANGOSBOT_ZERO LFG remains based on Penqle/Tortoise native `sLFGMgr` Meeting-Stone queue operations. The compatibility shim removal does not replace that with WotLK LFG. `LfgStrategy.cpp` is syntax-green in the completed Generic sweep.

The next LFG phase is intentionally separate: improve LivingBots dungeon/queue decision logic above the native queue. Shyalya's `World::GetLFGQueue()`/GetDungeonsForPlayer compatibility path is not adopted.
