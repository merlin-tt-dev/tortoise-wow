# LivingBots – Post-Clean-Port TODO

> Diese Punkte gehören **nicht** in den laufenden Clean-Port. Erst Clean-Port, Full Build und Runtime-Endtest abschließen; danach Erweiterungen auf der sauberen Basis aufbauen.

## Eluna – minimale, updatefähige Integration

- [ ] Eluna als **eigenes optionales Modul** integrieren; keine tief verteilten Lua-Änderungen im Tortoise-Core.
- [ ] Eluna-Upstream möglichst unverändert halten (Submodule/Subtree oder klar getrennte Vendor-Quelle); keine unnötige private Engine-Fork-Semantik.
- [ ] Nur die tatsächlich benötigten Tortoise-Hooks als kleinen, dokumentierten Core-Patch-Stack führen.
- [ ] Tortoise-spezifische API-Unterschiede hinter einem schmalen Adapter kapseln, statt Eluna-Upstream breit zu patchen.
- [ ] Build ohne Eluna muss weiterhin vollständig funktionieren.
- [ ] Normale Playerbots müssen ohne Eluna unverändert funktionieren.

## LivingBots – Lua Strategy Layer

- [ ] Playerbot-Lua-Anbindung als **separates Bridge-Modul** auf Eluna aufbauen; keine LivingBots-Logik in den Eluna-Core integrieren.
- [ ] Kontrollierte Lua-API für `Strategy`, `Trigger`, `Action` und bei Bedarf `Value` bereitstellen.
- [ ] Lua soll bevorzugt vorhandene C++-Actions/Values orchestrieren statt Movement, Spellcasting oder Core-State roh zu manipulieren.
- [ ] C++-Strategies bleiben autoritativer Fallback; Lua-Fehler dürfen Bots nicht deaktivieren oder den AI-Tick abbrechen.
- [ ] Lua-Ausführung budgetieren/cachen und bevorzugt event-/triggerorientiert ausführen; keine ungebremste Lua-Arbeit in jedem Bot-AI-Tick.
- [ ] Custom Strategies nach Rolle/Klasse/PvP/Dungeon/Persönlichkeit ohne Core-Neukompilierung ladbar machen.
- [ ] Hot-Reload nur dann ergänzen, wenn Lifecycle und Thread-Sicherheit sauber nachgewiesen sind.

## Reihenfolge

1. Clean-Port vollständig abschließen.
2. Clean Build und Runtime-Endtest durchführen.
3. Eluna minimal als separates, updatefähiges Modul integrieren.
4. LivingBots-Eluna-Bridge ergänzen.
5. Custom Lua Strategies und PvP-/Persönlichkeitslogik darauf aufbauen.
