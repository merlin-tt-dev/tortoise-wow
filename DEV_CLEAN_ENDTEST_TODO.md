# dev-clean – End-to-End Test TODO

> **Status:** Nur Testplan.
> **Regel:** `dev-clean` erst als endgültige Core-Baseline freigeben, wenn Build-, Start- und Runtime-Tests auf demselben dokumentierten Stand abgeschlossen sind.
> **Keine Modultests hier:** WorldOverlay, neues PlayerBots-Modul und andere Module werden auf ihren eigenen Branches getestet.

## Core-Patch Checkpoint (Runtime noch ausstehend)

| Patch / Bereich | Source-Status | Teststatus |
| --- | --- | --- |
| core-buildsystem-001 Revision-Header im Build-Tree | eingebaut | ausstehend |
| core-buildsystem-002 DPP/OpenSSL Debug-Linkage | eingebaut | ausstehend |
| core-buildsystem-003 Split mangosd Config Build Output | eingebaut | Build-/Config-Audit |
| core-buildsystem-004 Final Config Dist Layout | eingebaut | Build-/Install-Audit |
| core-buildsystem-005 Package Anticheat Config Dist | eingebaut | Build-/Runtime-Audit |
| core-only-001 Chat Channel Loaded ID | eingebaut | ausstehend |
| core-only-002 Chat Channel Name Match | eingebaut | ausstehend |
| core-only-003 LFT ObjectGuid Include | eingebaut | Build-Audit |
| core-only-004 SpellAuras Self-Contained Header | eingebaut | Build-Audit |
| core-only-005 Conditions Forward Declarations | eingebaut | Build-Audit |
| core-only-006 DPP Fixed-Width Integer Includes | eingebaut | Build-Audit |
| core-only-007 Discord GM Handler Disabled State | eingebaut / von 008 weitergeführt | Stack-/Build-Audit |
| core-only-008 Discord GM Commands Reactivation/Hardening | eingebaut | Runtime ausstehend |
| core-only-009 Model-Bounds GameObject Visibility | eingebaut | Runtime ausstehend |
| core-only-010 GObject Live Visibility/Spawn Commands + tmpadd Output | eingebaut | Runtime ausstehend |
| core-only-011 Restore Battleground Queue Locking | eingebaut | Build-/Threading-Runtime ausstehend |
| core-only-012 Config IncludeDir | eingebaut | Runtime ausstehend |
| core-only-013 Config Multi-Include / Active Switch / Duplicate Warnings | eingebaut | Runtime ausstehend |
| core-only-014 Config Dist Audit Cleanup | eingebaut | Config-/Runtime-Audit ausstehend |
| core-only-015 Anticheat Config Audit Fixes | eingebaut | Runtime ausstehend |
| core-only-016 Warden Enable Safety Gate | eingebaut | Build-/Runtime ausstehend |

**Wichtig:** „eingebaut“ bedeutet nur, dass der Source-Stand materialisiert ist. Kein Punkt gilt dadurch als runtime-getestet.

---

## 0. Testbasis festhalten

- [ ] finalen Commit-SHA des Teststands notieren.
- [ ] `dev-clean` als getesteten Branch dokumentieren.
- [ ] DB-/Genesis-Stand dokumentieren.
- [ ] verwendete `realmd.conf` / `mangosd.conf` sichern oder eindeutig zuordnen.
- [ ] Buildtyp festhalten (`Debug`, später optional `RelWithDebInfo`).
- [ ] verwendete Compiler-/CMake-/Ninja-Version notieren.
- [ ] verwendete Client-Version notieren.
- [ ] vollständige Serverlogs für Runtime-Tests aktivieren.
- [ ] bestätigen, dass keine Modulbranches oder lokalen Fremdpatches im Teststand enthalten sind.

**PASS:** Jeder Test ist exakt einer Commit-/DB-/Config-/Build-Kombination zuordenbar.

---

# A. Repository / Build / Start / Shutdown

## A1. Repository-Hygiene

- [ ] `git status --short` vor dem Audit leer.
- [ ] `git diff --check` leer.
- [ ] `patches/core/` enthält die erwartete geordnete Core-Patchserie.
- [ ] keine WorldOverlay-spezifischen Patches in `patches/core/`.
- [ ] keine neuen PlayerBots-Modulpatches in `patches/core/`.
- [ ] `MODULES=disabled` benötigt keine WorldOverlay-/mod-playerbots-Symbole.

**PASS:** reproduzierbarer, modulneutraler Working Tree.

## A2. Full Core Build Audit

Mit:

```bash
./core-full-build-audit.sh
```

Erwartete Audit-Basis:

- Debug
- `MODULES=disabled`
- `USE_PCH=OFF`
- Scripts ON
- Extractors OFF
- Discord/DPP ON

Prüfen:

- [ ] CMake Configure erfolgreich.
- [ ] vollständiger Ninja-Build erfolgreich.
- [ ] `realmd` linkt erfolgreich.
- [ ] `mangosd` linkt erfolgreich.
- [ ] keine Compilerfehler.
- [ ] keine Linkerfehler / `undefined reference`.
- [ ] keine neuen Warnungen, die auf Typ-, Lifetime-, Bounds- oder API-Probleme hindeuten.
- [ ] Build endet regulär bei allen Targets.
- [ ] `git status --short` nach Configure + Build weiterhin leer.
- [ ] kein generiertes `src/shared/revision.h` verschmutzt den Source-Tree.

**PASS:** kompletter Core-Audit auf exakt dem getesteten SHA grün und Source-Tree unverändert.

## A3. ccache

- [ ] vorhandenes `ccache` wird automatisch erkannt.
- [ ] Build funktioniert auch bei kaltem Cache.
- [ ] Folgebuild zeigt erwartbare Cache-Hits.
- [ ] Cache beeinflusst das Build-Ergebnis nicht.
- [ ] `USE_CCACHE=off` bleibt als funktionierender Fallback verfügbar.

**Hinweis:** Der erste Lauf mit leerem Cache darf fast vollständig aus Misses bestehen.

## A4. Serverstart

- [ ] `realmd` startet mit realer Testkonfiguration.
- [ ] `mangosd` startet mit realer Testkonfiguration.
- [ ] DB-Verbindungen erfolgreich.
- [ ] Welt-/Script-Initialisierung vollständig.
- [ ] keine Assertions.
- [ ] keine Crash-/Segfault-Meldungen.
- [ ] keine neuen DB-Fehler aus den Core-Patches.
- [ ] Realm ist für den Client sichtbar.
- [ ] Login bis Charakterauswahl funktioniert.
- [ ] Charakter kann die Welt betreten.

**PASS:** Server erreicht stabilen normalen Laufzustand.

## A5. Shutdown / Restart

- [ ] sauberer Shutdown von `mangosd`.
- [ ] sauberer Shutdown von `realmd`.
- [ ] erneuter Start ohne stale State.
- [ ] mindestens ein kompletter Restart mit zuvor eingeloggtem Charakter.
- [ ] keine neuen Fehler beim Map-/Grid-Unload während Shutdown.

**PASS:** mehrfacher Start/Stop ohne State-Leak oder Crash.

## A6. Anticheat / Warden Safe Baseline – core-only-015 / 016 + buildsystem-005

Sicherer Ausgangszustand für Turtle 1.18.1: das allgemeine Anticheat darf im Monitoring-Modus laufen; Warden bleibt deaktiviert, bis seine clientabhängigen Teile separat validiert sind.

### A6.1 Distribution / Startzustand

- [ ] Build erzeugt `anticheat.conf.dist`.
- [ ] ausgelieferte Dist enthält `Enable = 1`.
- [ ] ausgelieferte Dist enthält `Warden.Enable = 0`.
- [ ] Movement-Response-Actions im Safe-Profil sind nur `INFO_LOG` oder `NONE`; kein automatischer Kick/Ban durch die Dist-Defaults.
- [ ] `mangosd` startet mit kopierter `anticheat.conf.dist` als `anticheat.conf` ohne Configfehler.

### A6.2 Warden wirklich deaktiviert

Mit `Warden.Enable = 0`:

- [ ] normale Turtle-1.18.1-Anmeldung funktioniert bis Charakterauswahl.
- [ ] Charakterliste wird ohne Warden-Handshake unmittelbar gesendet.
- [ ] Charakter kann die Welt betreten.
- [ ] normale Bewegung erzeugt keinen Warden-Kick.
- [ ] eingehendes `CMSG_WARDEN_DATA` ohne Warden-Session wird ignoriert und verursacht weder Kick noch Crash.
- [ ] kein Warden-Challenge-/Modul-Handshake wird für die Session erzeugt.
- [ ] Movement/Fingerprint/Antispam bleiben unabhängig von Warden funktionsfähig.
- [ ] mindestens ein Reconnect bestätigt, dass der neue Sessionzustand reproduzierbar ist.

**PASS Safe Baseline:** Warden kann vollständig aus dem Client-Sessionpfad genommen werden, ohne Login, Character-Enum oder restliches Anticheat zu beschädigen.

### A6.3 Vor einer späteren Warden-Aktivierung zwingend offen

Diese Punkte sind **kein Bestandteil von core-only-016** und müssen separat auditiert/hardened werden:

- [ ] tatsächliche Turtle-1.18.1-Client-/World-Build-Semantik dokumentieren (`realmd` 7272 vs. WorldSession/Warden-Buildpfad 5875).
- [ ] alle festen Windows-Warden-Memory-/Code-Offsets gegen den real verwendeten Turtle-1.18.1-Client validieren.
- [ ] den **exakten** untersuchten Turtle-Clientstand festhalten: Clientversion, `WoW.exe`-SHA256, Dateigröße und PE-Timestamp; ein später geändertes Binary gilt bis zur erneuten Validierung als unbekannter Client.
- [ ] soweit verfügbar einen originalen Vanilla-1.12.1.5875-Client als Referenz verwenden, um jeden alten Hardcoded Offset zuerst semantisch zu identifizieren: welche Funktion, globale Struktur oder welcher Zustand wurde dort tatsächlich gelesen?
- [ ] die korrespondierenden Turtle-1.18.1-Stellen per statischer Binaryanalyse (z. B. Ghidra/IDA) anhand von String-Xrefs, Callgraph, Imports, Konstanten, Datenstrukturen und charakteristischen Instruktionsfolgen neu identifizieren; alte numerische Adressen niemals nur übernehmen.
- [ ] gefundene Kandidaten anschließend dynamisch mit einem lokalen Testclient/Debugger verifizieren: kontrollierte Zustandsänderungen wie Bewegung, Sprung, Schwimmen, Tracking oder Click-to-Move müssen exakt mit dem vermuteten Feld bzw. der vermuteten Funktion korrelieren.
- [ ] validierte Clientadressen als modulrelative RVA/Offset-Daten dokumentieren; keine nackten absoluten Prozessadressen als dauerhaft gültige Wahrheit behandeln.
- [ ] für **jeden einzelnen Warden-Scan** einen expliziten Kompatibilitätszustand führen:
  - `SUPPORTED` = für exakt den dokumentierten Client-Binary-Hash statisch und dynamisch validiert,
  - `UNRESOLVED` = Ziel/Funktion noch nicht sicher identifiziert,
  - `UNSUPPORTED` = Scan ist für diesen Client nicht sinnvoll oder nicht portiert.
- [ ] nur `SUPPORTED`-Scans dürfen überhaupt einen clientseitigen Memory-/Code-Check erzeugen; `UNRESOLVED` und `UNSUPPORTED` werden übersprungen und geloggt.
- [ ] Signatur-/Resolver-Ergebnisse zusätzlich auf erwartete Instruktions-/Datenstrukturform prüfen; kein Scan darf allein deshalb aktiv werden, weil eine Byte-Signatur zufällig genau einen Treffer liefert.
- [ ] für jeden unterstützten Client-Binary-Hash eine nachvollziehbare Warden-Kompatibilitätsmatrix pflegen (z. B. GetText, File-API, MovementFlags, MoveSpeed, Tracking, ClickToMove, Warden-Internals, Hook-Checks).
- [ ] unbekannter oder nach einem Turtle-Update veränderter Client-Hash muss Warden für diese Session automatisch deaktivieren bzw. alle clientabhängigen Scans überspringen; **kein Kick/Ban nur wegen fehlendem Profil, nicht auflösbarer Signatur oder Versionsdrift**.
- [ ] nach jedem Turtle-Clientupdate gelten alle binaryabhängigen Warden-Scans bis zur erneuten Validierung mindestens als `UNRESOLVED`; keine stillschweigende Übernahme der Freigabe vom vorherigen Client.
- [ ] vorhandene Warden-Module (`.bin/.key/.cr`) und deren Protokollkompatibilität mit Turtle 1.18.1 validieren; keine Module nur aufgrund alter Classic-Kompatibilität aktivieren.
- [ ] alle direkten `KickPlayer()`-Pfade im Warden-Protokoll einzeln klassifizieren: Challenge, Checksum, unbekanntes Opcode, Module-Failure, unsupported Build/OS.
- [ ] Warden-Protokollfehler standardmäßig fail-open/log-only machen oder hinter eine explizite Enforcement-Option stellen.
- [ ] unsupported/unerwarteter Client-Build darf nicht allein durch Versionsdrift einen Massenkick verursachen.
- [ ] Character-Enum bleibt auch bei fehlendem/inkompatiblem Warden-Modul fail-open.
- [ ] malformed Warden-Pakete, Timeout und Challenge-Fehler mit Testclient gezielt prüfen.
- [ ] Windows- und gegebenenfalls Mac-Pfad getrennt testen.
- [ ] Reload-Semantik von `Warden.Enable` dokumentieren und testen; bestehende Sessions dürfen keinen inkonsistenten Zwischenzustand erhalten.
- [ ] erst nach diesen Punkten Warden auf einem Testrealm aktivieren; Enforcement erst nach sauberem Monitorlauf.

---

# B. Core Smoke Test

Dieser Block stellt sicher, dass `dev-clean` als echte Null-Linie für spätere Modulbranches taugt.

- [ ] Login / Logout.
- [ ] Charakter bewegen, springen, schwimmen.
- [ ] normale Teleports / Hearthstone.
- [ ] Zauber wirken, Channeling, Auren setzen/entfernen.
- [ ] Nahkampf und Spell-Damage.
- [ ] Loot eines normalen NPCs.
- [ ] Item benutzen / ausrüsten / ablegen.
- [ ] Quest annehmen / Fortschritt / Abgabe als Stichprobe.
- [ ] Gruppe erstellen / verlassen.
- [ ] Whisper, Say, Party, Guild und Standardchannel als Stichprobe.
- [ ] mindestens ein normaler Dungeon-Enter/Leave.
- [ ] normale GameObjects benutzen: Tür, Truhe oder vergleichbares GO.
- [ ] kein auffälliger CPU-/Speicheranstieg im einfachen Idle-/Spielbetrieb.

**PASS:** keine generische Core-Regression erkennbar.

---

# C. Chat Channels – core-only-001 / 002

## C1. Geladene Channel-ID

- [ ] mehrere Standardchannels aus `chat_channels` laden.
- [ ] `General` erhält seine echte DB-ID.
- [ ] `Trade` erhält seine echte DB-ID.
- [ ] `LocalDefense` erhält seine echte DB-ID.
- [ ] `LookingForGroup` erhält seine echte DB-ID.
- [ ] unterschiedliche Built-in-Channels erhalten nicht mehr pauschal `id = 1`.
- [ ] Join/Leave funktioniert weiterhin normal.

## C2. Channel-Name-Matching

- [ ] fixer Channelname ohne `%s` matcht nur exakt.
- [ ] zonenabhängiger Name mit `%s` matcht gültigen Prefix + Suffix.
- [ ] `General - <Zone>` wird als korrekter Built-in-Channel erkannt.
- [ ] ein String, der den Namen nur irgendwo als Substring enthält, wird **nicht** als Built-in-Channel erkannt.
- [ ] Shortcuts matchen weiterhin exakt.
- [ ] mindestens ein lokalisierter Channelname funktioniert, soweit die Test-DB ihn bereitstellt.
- [ ] selbst angelegter Custom-Channel bleibt Custom-Channel.

**PASS:** korrekte Channel-ID und keine falschen Built-in-Matches.

---

# D. Discord / DPP – core-only-006 / 007 / 008 + buildsystem-002

## D1. Build-/Linkpfad

- [ ] Debug-Build mit Discord/DPP linkt `realmd` ohne OpenSSL-Fehler.
- [ ] `mangosd` linkt ebenfalls sauber.
- [ ] kein `DSO missing from command line`.
- [ ] `DISCORD_DEBUG=0` erzeugt keinen Auth-Bypass.

## D2. Bot-Startup / Shutdown

- [ ] Discord-Bot startet bei aktivierter Konfiguration.
- [ ] Verbindung wird erfolgreich aufgebaut.
- [ ] sauberer Server-Shutdown beendet auch den Bot-Pfad.
- [ ] Neustart verbindet erneut ohne doppelten Handler-/Command-State.

## D3. Auth / Commands

- [ ] nicht authentifizierter Discord-User darf GM-Kommandos nicht ausführen.
- [ ] authentifizierter User mit unzureichendem Security-Level wird abgewiesen.
- [ ] berechtigter Staff-User kann die aktivierten GM-Kommandos verwenden.
- [ ] Textcommand mit `.`-Prefix wird korrekt normalisiert.
- [ ] Textcommand mit `/`-Prefix wird korrekt normalisiert.
- [ ] Command-Namen werden für Access-Checks korrekt lowercase behandelt.

## D4. `.logs`

- [ ] gültiger Logtyp + gültige Länge liefert Inhalt.
- [ ] ungültiger negativer Logtyp wird sicher abgewiesen.
- [ ] Logtyp `>= LOG_MAX_FILES` wird sicher abgewiesen.
- [ ] `numChars <= 0` wird sicher abgewiesen.
- [ ] sehr großer Wert wird auf Discord-Maximallänge begrenzt.
- [ ] Lesen laufender Logs verursacht keinen Crash / kaputten File-Offset.
- [ ] paralleles Serverlogging bleibt intakt.

## D5. Erwarteter Zustand `.lookup`

- [ ] `.lookup` ist **absichtlich nicht registriert**.
- [ ] das wird nicht als Regression gewertet, solange der Component-Selection-Handler fehlt.

**PASS:** Discord funktioniert ohne Auth-Bypass, Linkerproblem oder Log-Reader-Crash.

---

# E. Large GameObject / WMO Visibility – core-only-009

Bekannter Referenzfall: sehr großes historisches WMO wie Old/Alpha Ironforge, dessen Template nicht zuverlässig als `large` markiert ist und dessen Modell mehrere Sicht-/Gridbereiche überragen kann.

## E1. Automatische Model-Bounds-Erkennung

- [ ] Referenz-WMO ohne manuell erhöhte `visibility_mod` laden.
- [ ] reale VMAP-/Model-Bounds führen bei physisch großem Modell automatisch zu erhöhter Sichtweite.
- [ ] Legacy-Template mit `large = 0` kann trotzdem über Model-Bounds erkannt werden.
- [ ] kleines normales GO erhält **keine** unnötige Large-Visibility.
- [ ] mittelgroße normale Gebäude/GOs zeigen keine auffällige Verhaltensänderung.

## E2. Sichtbarkeit über große Distanz / Gridgrenzen

- [ ] Referenz-WMO aus mehreren Richtungen annähern.
- [ ] bekannte alte Pop-in-/Disappear-Grenze überschreiten.
- [ ] WMO bleibt sichtbar, solange seine reale Modellgeometrie relevant ist.
- [ ] mindestens zwei betroffene Gridbereiche praktisch durchlaufen.
- [ ] wegbewegen und erneut annähern.
- [ ] teleportieren und danach erneut annähern.

## E3. Cold-Load / Restart

- [ ] Server frisch starten und direkt zum Referenz-WMO gehen.
- [ ] Test durchführen, bevor sein Spawn-Origin-Grid absichtlich vorher besucht wurde.
- [ ] Map verlassen / unloaden lassen, danach erneut betreten.
- [ ] nach vollständigem Serverrestart erneut testen.

Falls der Fix erst funktioniert, nachdem das Spawn-Origin-Grid einmal geladen wurde, separat als **Grid-Preload-/Spawn-Load-Problem** dokumentieren; nicht mit dem Model-Bounds-Fix vermischen.

## E4. Regression / Performance

- [ ] keine Massenaktivierung kleiner GameObjects.
- [ ] keine auffällige CPU-Spitze durch normale GO-Visibility.
- [ ] kein Crash beim Grid-Unload eines automatisch groß erkannten GO.
- [ ] Collision/LOS des Referenz-WMO weiterhin normal.
- [ ] normale `large`-/`infinite`-GameObjects behalten ihr bisheriges Verhalten.

**PASS:** riesige Modelle werden anhand ihrer realen Bounds zuverlässig sichtbar, ohne normale GOs zu beeinträchtigen.

---

# F. GObject Runtime-/Admin-Commands – core-only-010

## F1. `.gobject tmpadd`

- [ ] `.gobject tmpadd <entry>` erzeugt weiterhin ein temporäres GO.
- [ ] Chat-Ausgabe enthält Entry.
- [ ] Chat-Ausgabe enthält Name.
- [ ] Chat-Ausgabe enthält Runtime-GUID.
- [ ] Chat-Ausgabe enthält X/Y/Z.
- [ ] optional angegebene Spawnzeit funktioniert weiterhin.
- [ ] temporäres GO erzeugt **keine** persistente `gameobject`-DB-Zeile.
- [ ] temporäres GO verschwindet wie vorgesehen.
- [ ] Serverrestart bringt das temporäre GO nicht zurück.

## F2. `.gobject set visibility`

Mit ausgewähltem persistentem GO:

- [ ] `.gobject set visibility <wert>` wirkt sofort live.
- [ ] `.gobject set visi <wert>` funktioniert als Abkürzung.
- [ ] `gameobject.visibility_mod` wird persistent aktualisiert.
- [ ] ObjectMgr-Runtime-Cache enthält denselben Wert.
- [ ] Respawn/Reload verwendet den neuen Wert weiter.
- [ ] Serverrestart verwendet den neuen Wert weiter.
- [ ] `visibility 0` wird akzeptiert.
- [ ] negativer Wert wird abgewiesen.
- [ ] ungültiger / nichtnumerischer Wert wird sauber abgewiesen.
- [ ] NaN/Inf gelangt nicht als gültiger Visibility-Wert in den Runtime-State.

## F3. `.gobject set spawn_flags`

- [ ] `.gobject set spawn_flags <mask>` ändert den persistenten Spawn-Mask-Wert.
- [ ] ACTIVE-Bit wirkt unmittelbar auf den Runtime-ActiveObject-State.
- [ ] Entfernen des ACTIVE-Bits wirkt unmittelbar.
- [ ] DB `gameobject.spawn_flags` entspricht dem gesetzten Wert.
- [ ] ObjectMgr-Runtime-Cache entspricht dem gesetzten Wert.
- [ ] Restart behält den Wert.

## F4. `.gobject set spell_focus` Alias

- [ ] `.gobject set spell_focus <mask>` verhält sich identisch zu `spawn_flags`.
- [ ] Bestätigungsausgabe ist verständlich.
- [ ] der Alias verändert **nicht** `gameobject_template.data0` / die eigentliche SpellFocus-ID.

**Erwartetes Verhalten:** `spell_focus` ist hier bewusst ein Kompatibilitätsalias für `gameobject.spawn_flags`, weil dieser Name für den Live-Korrekturworkflow gewünscht wurde.

## F5. Setter auf temporärem GO

- [ ] `visibility` wirkt auf einem `tmpadd`-GO live.
- [ ] `spawn_flags` / `spell_focus` wirkt soweit runtime-seitig möglich live.
- [ ] Ausgabe sagt ausdrücklich `runtime only` / keine DB-Zeile.
- [ ] es wird niemals versehentlich eine persistente Spawn-Zeile für `tmpadd` angelegt.

## F6. `.gobject info`

- [ ] zeigt Visibility Modifier nach Änderung plausibel an.
- [ ] zeigt Active Object nach gesetztem ACTIVE-State plausibel an.
- [ ] GUID/Entry stimmen mit dem bearbeiteten Objekt überein.

**PASS:** alle neuen Adminpfade wirken live, persistieren nur bei echten DB-Spawns und verändern keine falschen Template-Felder.

---

# G. Patchserie / Reproduzierbarkeit

- [ ] Core-Patches lassen sich in dokumentierter Reihenfolge auf die vorgesehene Basis anwenden.
- [ ] jeder Patch besteht vor Anwendung `git apply --check`.
- [ ] nach kompletter Anwendung `git diff --check` sauber.
- [ ] `007`/`008` werden als abhängiger Discord-Stack behandelt.
- [ ] echter Reverse-Stack wird von hinten tatsächlich angewendet, wenn die Rückbaubarkeit getestet wird.
- [ ] nicht erwarten, dass `git apply --check -R 007` direkt gegen den finalen 008-State matcht, ohne 008 vorher wirklich zurückzunehmen.
- [ ] Patch-Artefakte und materialisierter Source-State beschreiben dieselben Änderungen.

**PASS:** `dev-clean` ist aus Basis + Patchserie reproduzierbar.

---

# H. Modulneutralität / Baseline-Freeze

- [ ] Full Core Audit mit `MODULES=disabled` grün.
- [ ] kein WorldOverlay-Runtimepfad nötig, um den Core zu starten.
- [ ] kein neues `modules/mod-playerbots` nötig, um den Core zu starten.
- [ ] keine AdminCommands-Modulabhängigkeit.
- [ ] bekannte Legacy-Dateien unter `src/game/PlayerBots/` separat klassifiziert; nicht blind mit dem neuen Modul gleichgesetzt.
- [ ] `dev-clean`-TODOs gegen den tatsächlichen Endstand aktualisieren.
- [ ] finalen getesteten SHA in dieser Datei oder Release-/Branch-Doku festhalten.

**PASS:** der Branch ist eine belastbare, modulneutrale Ausgangsbasis für alle `module-*`-Branches.

---

# I. Endfreigabe

Erst abhaken, wenn alle für `dev-clean` relevanten Blöcke bestanden sind:

- [ ] Build Gate grün.
- [ ] Start/Shutdown grün.
- [ ] Core Smoke Test grün.
- [ ] Chat-Channel-Fixes grün.
- [ ] Discord Runtime/Security grün oder bewusst als nicht verwendetes optionales Feature mit dokumentiertem Zustand akzeptiert.
- [ ] Large-WMO-Visibility grün.
- [ ] GObject-Commands grün.
- [ ] Anticheat Safe Baseline grün; Warden bleibt deaktiviert oder ist separat vollständig gegen Turtle 1.18.1 validiert.
- [ ] Repository-Hygiene grün.
- [ ] finaler Test-SHA dokumentiert.

## Bekannte erwartete Zustände – nicht als Fehler werten

- Discord `.lookup` bleibt vorerst absichtlich unregistriert.
- `.gobject set spell_focus` ist Alias für `spawn_flags`, nicht der SpellFocus-Templatewert.
- `.gobject tmpadd` meldet eine Runtime-GUID und erzeugt keine DB-GUID.
- ein kalter `ccache` darf beim ersten Build fast nur Misses zeigen.
- `Warden.Enable = 0` ist der erwartete sichere Default, solange die Turtle-1.18.1-spezifische Warden-Kompatibilität nicht vollständig validiert ist.
- `core-only-007` kann nicht isoliert gegen den bereits von `008` weiterveränderten finalen Tree rückwärts geprüft werden; für einen echten Reverse-Test zuerst spätere Patches tatsächlich rückwärts anwenden.

---

## Regel bei Fehlern

Keine spekulativen Sammelfixes.

Wenn ein Test oder Build fällt:

1. ersten echten Fehler bestimmen,
2. Folgefehler davon trennen,
3. Ownership bestimmen,
4. minimal reparieren,
5. betroffenen Test wiederholen,
6. danach das vollständige relevante Gate erneut laufen lassen.

So bleibt `dev-clean` die diagnostische Null-Linie für WorldOverlay, PlayerBots und alle weiteren Module.