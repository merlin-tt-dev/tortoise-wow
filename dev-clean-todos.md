# dev-clean TODOs

Stand: 2026-09-07

Zweck dieser Datei: laufende Fehler-, Audit- und Aufräumliste für `dev-clean`.

`dev-clean` ist die modulneutrale Core-Baseline. Neue Punkte werden nach dem Prinzip
**erster echter Fehler zuerst** aufgenommen und erst nach sauberem Full-Build-Audit
als erledigt markiert.

---

## Offen – hohe Priorität

### 1. Discord GM Commands sauber reaktivieren

Aktueller Zustand:

- `src/game/DiscordBot/GMCommandHandler.cpp` enthielt einen kaputten auskommentierten Block.
- `GMCommandHandler::IsAuthorized()` und `GMCommandHandler::RegisterCommands()` waren teilweise
  auskommentiert.
- Der Kommentar endete vor einer schließenden Klammer, wodurch `namespace DiscordBot`
  versehentlich zu früh geschlossen wurde.
- Dadurch entstanden u. a.:
  - `GMCommandHandler has not been declared`
  - `AuthManager has not been declared`
  - `_commandOutput was not declared`
  - `invalid use of this in non-member function`
  - `MaxMessageLength` außerhalb des Namespaces
- Der aktuelle Core-Fix hält die kaputten GM-Kommandos bewusst vollständig deaktiviert,
  damit der Core wieder sauber gebaut werden kann.

TODO:

- `GMCommandHandler::IsAuthorized()` sauber wieder implementieren.
- `GMCommandHandler::RegisterCommands()` sauber wieder implementieren.
- `gm`, `logs` und `lookup` einzeln prüfen.
- Security-Level/Autorisierung verifizieren.
- Keine halbfertigen Kommentarblöcke als Feature-Schalter verwenden.
- Discord-GM-Kommandos erst wieder aktivieren, wenn Compile + Link + Runtime-Test sauber sind.

### 2. Discord `logs` Command darf nicht auf private `Log`-Interna zugreifen

Bekannter Defekt:

`GMCommandHandler::LogCommand()` greift direkt auf private Member von `Log` zu:

- `Log::logFiles`
- `Log::logLock`

Der Compiler meldet korrekt, dass beide `private` sind.

TODO:

- Keine `friend`-Abkürzung nur für Discord einbauen.
- Saubere öffentliche, read-only Log-API entwerfen, falls der Discord-`logs`-Command
  erhalten bleiben soll.
- Alternativ `logs`-Command entfernen/deaktiviert lassen, falls keine saubere API sinnvoll ist.
- Bounds-Check für angeforderten `logType`.
- Plausibilitäts-/Größenlimit für `numChars`.
- Thread-Safety beim Lesen laufender Logdateien sicherstellen.

### 3. `dev-clean` Full-Build-Audit vollständig grün bekommen

Audit-Skript:

`./core-full-build-audit.sh`

Erwartete Baseline:

- Debug
- PCH OFF
- Module disabled
- Scripts ON
- Extractors OFF
- Discord/DPP ON

Vorgehen bei jedem Fehlschlag:

1. ersten echten Compiler-/Linkerfehler bestimmen
2. Root Cause von Folgefehlern trennen
3. klassifizieren:
   - generischer Core
   - vendored dependency
   - Buildsystem
   - unbeabsichtigte Modulabhängigkeit
4. minimalen Patch erstellen
5. `git apply --check`
6. Patch anwenden
7. `git diff --check`
8. committen
9. Audit von vorn starten

Erst wenn der Full-Build komplett grün ist, gilt der Commit als belastbare
`dev-clean`-Baseline.

---

## Prüfen / klassifizieren

### 4. Legacy `src/game/PlayerBots/*` im Core-Build prüfen

Im aktuellen Core-only Full-Build mit deaktivierten Modulen werden trotzdem u. a. gebaut:

- `src/game/PlayerBots/PlayerBotAI.cpp`
- `src/game/PlayerBots/PlayerBotMgr.cpp`

Das ist unabhängig vom neuen `modules/mod-playerbots`.

TODO:

- Klären, ob diese Dateien absichtlicher Bestandteil des Tortoise-Core sind.
- Prüfen, welche Funktionen sie bereitstellen und ob sie vom normalen Core benötigt werden.
- Prüfen, ob sie historische/legacy PlayerBot-Reste sind.
- Nicht blind entfernen.
- Falls sie ausschließlich wegen PlayerBots existieren:
  - Ownership sauber bestimmen.
  - aus `dev-clean` herauslösen oder hinter eine saubere optionale Grenze bringen.
- Falls sie echter Tortoise-Core sind:
  - dokumentieren, warum sie trotz `MODULES=disabled` gebaut werden.

### 5. Discord-Bot Core-Pfad vollständig auditieren

Der erste DPP-Fehler wurde bereits behoben, danach wurde der kaputte
`GMCommandHandler.cpp` sichtbar.

TODO:

- Nach jedem Fix Full-Build erneut starten.
- Weitere DiscordBot-Dateien nicht vorsorglich ändern.
- Erst reagieren, wenn ein echter Buildfehler erreicht wird.
- Nach grünem Compile auch auf Linkfehler achten.
- Später im `integrated-all` Runtime-Test:
  - Bot-Startup
  - Auth
  - normale Commands
  - deaktivierte/reaktivierte GM-Commands
  - Shutdown/Stop-Pfad

### 6. Module-Neutralität von `dev-clean` weiter prüfen

`dev-clean` darf keine WorldOverlay- oder neue PlayerBots-Modulabhängigkeit voraussetzen.

Prüfpunkte:

- keine WorldOverlay-spezifische Unbound-Dungeon-API in `dev-clean`
- keine `modules/mod-worldoverlay`-Abhängigkeit
- keine `modules/mod-playerbots`-Abhängigkeit
- generische Fixes nur in `patches/core/`
- modulmotivierte Core-Hooks ausschließlich unter
  `patches/mod-<name>/core/`

---

## Repository-/Build-Hygiene

### 7. `revision.h` Buildsystem-Fix verifizieren

Bereits angewendet:

- `revision.h` wird im Build-Tree generiert.
- Source-Tree soll beim CMake-Configure nicht mehr verändert werden.

Noch prüfen:

- nach komplettem Configure + Build `git status` leer
- kein generiertes `src/shared/revision.h` als Working-Tree-Änderung
- mehrere frische Builds reproduzierbar

### 8. Alte große `tmp`-Datei in Git-Historie optional bereinigen

Historisch wurde kurzzeitig eine große Datei committed:

`tmp/tortoise-wow-mod-worldoverlay.tar.xz`

Sie wurde danach wieder aus dem aktuellen Tree entfernt, steckt aber weiterhin in der
Branch-Historie.

TODO, optional vor endgültigem Baseline-Freeze:

- entscheiden, ob History-Rewrite sinnvoll ist
- nur `dev-clean`-Historie bereinigen, wenn dadurch keine wichtigen Abhängigkeiten beschädigt werden
- danach abhängige Modulbranches kontrolliert neu aufsetzen

Kein funktionaler Build-Blocker.

---

## Bereits erledigt / verifiziert

### Generische Core-Fixes

- [x] Conditions Forward Declarations
- [x] LFT `ObjectGuid` Include
- [x] `SpellAuras` self-contained Header
- [x] DPP fixed-width integer includes (`<cstdint>`)
- [x] `revision.h` in den Build-Tree verlagert

### WorldOverlay-Ownership korrigiert

- [x] Unbound-Dungeon Transfer API aus `dev-clean` revertiert
- [x] WorldOverlay-owned Unbound-Dungeon-Patches aus `patches/core/` entfernt
- [x] diese Änderungen werden beim Neuaufbau nach
      `patches/mod-worldoverlay/core/` verschoben

### Discord GM Handler – temporärer sicherer Zustand

- [x] kaputten Kommentar-/Namespace-Zustand als Root Cause identifiziert
- [x] GM-Commands bleiben vorerst bewusst deaktiviert
- [ ] saubere Reaktivierung steht noch aus

---

## Nächste Baseline-Gates

### Gate A – `dev-clean`

- [ ] `core-full-build-audit.sh` komplett grün
- [ ] `git status` danach leer
- [ ] `git diff --check` leer
- [ ] finalen grünen Commit als Baseline festhalten

### Gate B – einzelne Modulbranches

Danach jeweils neu aus der grünen `dev-clean`-Baseline aufbauen:

- [ ] `module-worldoverlay`
- [ ] `module-playerbots`
- [ ] weiteres Admin-Commands-Modul

Für jedes Modul:

- eigener Patch-Namespace
- eigener vollständiger Build-Audit
- kein unbeabsichtigter Fremdmodul-Code

### Gate C – `integrated-all`

Erst wenn alle Einzelbranches grün sind:

- [ ] Module gemeinsam integrieren
- [ ] Full Build
- [ ] Serverstart
- [ ] Runtime-/Ingame-Tests
- [ ] Fehler, die nur hier auftreten, als Integration klassifizieren

---

## Regel für neue TODOs

Keine spekulativen Reparaturen.

Wenn ein Build fällt:

> Den ersten echten Fehler dokumentieren, klassifizieren, minimal reparieren und erneut vollständig bauen.

So bleibt `dev-clean` die diagnostische Null-Linie für alle späteren Module.
