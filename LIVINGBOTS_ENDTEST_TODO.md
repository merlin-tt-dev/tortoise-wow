# LivingBots Clean Port – End-to-End Test TODO

> **Status:** Nur Testplan.  
> **Regel:** Erst ausführen, wenn der Clean-Port vollständig abgeschlossen ist.  
> **Keine Zwischenabnahme einzelner Patches.**

## Source-Port Checkpoint (noch ungetestet)

| Portblock | Source-Status | Runtime-Test |
| --- | --- | --- |
| 0001–0010 Clean-Port Basis/Fixes | eingebaut | ausstehend |
| 0011 Static Transport GameObject GUID | eingebaut | ausstehend |
| 0012 Open-Lock: alle Spell-Effects prüfen | eingebaut | ausstehend |
| 0013 Mining/Herbalism auf echtes GameObject | eingebaut | ausstehend |
| 0014 Native Penqle Loot Access | eingebaut | ausstehend |
| 0015 Native Quest-Relation Access | eingebaut | ausstehend |
| 0016 Native Travel Metadata | eingebaut | ausstehend |
| 0017 Native Config IncludeDir | eingebaut | ausstehend |
| 0018 Native Compatibility Semantics I | eingebaut | ausstehend |
| 0019 Native Stores / AreaTriggers / Channels / Pet Autocast | mit diesem Patch | ausstehend |

**Wichtig:** „eingebaut“ bedeutet nur statisch/source-seitig portiert. Kein Punkt gilt dadurch als getestet.

---

## 0. Testbasis festhalten

- [ ] Finalen Commit-SHA des Teststands notieren.
- [ ] Verwendete Tortoise-Basis (`1181dev`) und LivingBots-Branch dokumentieren.
- [ ] DB-Stand / Genesis-Stand dokumentieren.
- [ ] verwendete `aiplayerbot.conf` + `aiplayerbot.d/` sichern.
- [ ] Server-Buildtyp festhalten (`Debug` / `RelWithDebInfo` / etc.).
- [ ] vollständige Serverlogs für jeden Testlauf aktivieren.
- [ ] Testpopulation und Account-Prefix dokumentieren.
- [ ] Testzeitpunkt und verwendete Client-Version notieren.

**PASS:** Ein Testlauf ist exakt reproduzierbar und einer Commit-/Config-/DB-Kombination zuordenbar.

---

# A. Build / Start / Shutdown

## Compatibility-/Shim-Audit

- [ ] 0018: Bewegungsstatus verwendet Penqles `MOVEFLAG_MASK_MOVING`, nicht eine All-Bits-Maske.
- [ ] 0018: `UNIT_FLAG_DISABLE_MOVE` blockiert Bot-Bewegung an den ehemaligen `CLIENT_CONTROL_LOST`-Stellen.
- [ ] 0018: Spell-Attribute/Interrupt-/Target-Flags verwenden Penqles native 1.12-Symbole und Bitwerte.
- [ ] 0018: Unique-equipped- und unlearnable-Skill-Prüfungen verwenden Penqles native Flags.
- [ ] 0018: unsichtbare Creature-Templates werden über `flags_extra` / `CREATURE_FLAG_EXTRA_INVISIBLE` gefiltert.
- [ ] 0018: Channeling wird über Penqles nativen `SPELL_STATE_CASTING`-Zustand erkannt.
- [ ] 0018: GossipHello läuft über `sScriptMgr`; kein No-op-Compatibility-Hook.
- [ ] 0019: Item-/Creature-/GameObject-/Faction-Scans iterieren Penqles tatsächlich geladene ObjectMgr-Maps ohne künstliche ID-Hardlimits.
- [ ] 0019: AreaTrigger-Geometrie und Teleport-Metadaten verwenden getrennt `AreaTriggerEntry` und `AreaTriggerTeleport`.
- [ ] 0019: AreaTrigger-Conditions verwenden Penqles `CONDITION_FROM_AREATRIGGER`-Semantik.
- [ ] 0019: Standard-ChatChannels werden aus Penqles geladenen `chat_channels`-Metadaten aufgebaut; kein leerer DBC-Store-Shim.
- [ ] 0019: Pet-Autocast wird nicht mehr global durch `IsAutocastable() == false` deaktiviert.
- [ ] core-chat-channel-loaded-id: `LoadChatChannels()` erhält die echte DB-ID jedes Standardchannels.
- [ ] core-chat-channel-name-match: feste Channel-Namen matchen exakt; zonenabhängige Namen matchen das `%s`-Pattern ohne Substring-Kollisionen.
- [ ] Rest-Shim-Audit (MMap/BG/Loot/LFG/InstanceTemplate/etc.) vollständig abgeschlossen.

## A1. Clean Build

- [ ] vollständiger Clean Build ohne Alt-Artefakte.
- [ ] keine Compilerfehler.
- [ ] keine neuen Warnungen aus `mod-playerbots`, die auf API-/Typ-/Lifetime-Probleme hindeuten.
- [ ] keine Linkerfehler.
- [ ] keine ODR-/duplicate-symbol-Probleme durch Compatibility-Shims.

**PASS:** kompletter Build erfolgreich.

## A2. Serverstart

- [ ] Server startet mit Playerbots aktiviert.
- [ ] Playerbot-Modul initialisiert vollständig.
- [ ] Travel-Daten initialisieren.
- [ ] Loot-/Spell-/Strategy-Initialisierung vollständig.
- [ ] keine Assertions.
- [ ] keine null-pointer / invalid GUID / DB-query exceptions.
- [ ] keine Endlosschleife beim Startup.
- [ ] keine auffälligen Initialisierungszeiten durch DBC-/SQL-Proxy-Scans.

**PASS:** Server erreicht normalen laufenden Zustand.

## A3. Shutdown / Restart

- [ ] sauberer Shutdown mit Bots online.
- [ ] sauberer Neustart.
- [ ] keine stale Bot-Sessions.
- [ ] keine doppelten Bot-Objekte nach Restart.
- [ ] keine beschädigten Playerbot-DB-Events.

**PASS:** mehrfacher Restart ohne State-Leak.

---

# B. Config-System

## B1. Grundconfig

- [ ] normale Einzelwerte werden korrekt gelesen.
- [ ] Boolean / Integer / Float / String.
- [ ] Defaultwerte greifen korrekt.
- [ ] ungültige Werte erzeugen verständliche Logs.

## B2. Multi-Value Config

- [ ] `LoginCriteria`.
- [ ] `WorldBuff`.
- [ ] weitere `GetValues()`-Nutzer.
- [ ] keine stillen leeren Listen.

## B3. Include-System

Falls bis dahin implementiert:

- [ ] `aiplayerbot.conf` lädt `aiplayerbot.d/`.
- [ ] lexikographische Reihenfolge korrekt.
- [ ] `10-...` vor `90-...`.
- [ ] spätere Werte überschreiben frühere nur wie vorgesehen.
- [ ] fehlende optionale Include-Datei nicht fatal.
- [ ] Syntaxfehler klar geloggt.
- [ ] Include-Rekursion / Include-Loop verhindert.

## B4. Compatibility Hacks

- [ ] alle Recovery-/Teleport-Hacks standardmäßig `0`.
- [ ] Normalbetrieb verwendet keinen Hackpfad.
- [ ] optional aktivierter Hack funktioniert getrennt vom Normalpfad.

**PASS:** Config entspricht Dateiinhalt und kein Legacy-Hack ist unbeabsichtigt aktiv.

---

# C. Bot-Erstellung / Rassen / Klassen

## C1. Vanilla-Rassen

- [ ] Human
- [ ] Orc
- [ ] Dwarf
- [ ] Night Elf
- [ ] Undead
- [ ] Tauren
- [ ] Gnome
- [ ] Troll

## C2. Turtle-Rassen

- [ ] High Elf
- [ ] Goblin

Für jede Rasse:

- [ ] zulässige Klassen korrekt.
- [ ] Erscheinungsdaten gültig.
- [ ] Startposition korrekt.
- [ ] Homebind korrekt.
- [ ] Faction / Team korrekt.
- [ ] Startspells korrekt.
- [ ] Startskills korrekt.
- [ ] Sprache(n) korrekt.
- [ ] Startquests erreichbar.
- [ ] keine Human-/Orc-Fallback-Semantik für High Elf/Goblin.

**PASS:** jeder erzeugte Bot entspricht `playercreateinfo` und Turtle-Daten.

---

# D. Login / Population / Persistenz

## D1. Soll-/Ist-Population

- [ ] gewünschte Botzahl entspricht real eingeloggter Botzahl.
- [ ] interne Count-/Cache-Werte entsprechen realen Sessions.
- [ ] kein Zustand „Server glaubt genug Bots online, obwohl sie fehlen“.

## D2. Login/Logout

- [ ] Bots loggen zuverlässig ein.
- [ ] Bot-Logout reduziert Counts korrekt.
- [ ] erneutes Login funktioniert.
- [ ] keine Ghost-Sessions.
- [ ] keine doppelten Sessions.

## D3. Restart-Persistenz

- [ ] Botzustand bleibt über Serverrestart plausibel.
- [ ] kein ungewolltes Level-/Position-/Inventory-Reset.
- [ ] keine alten RandomBot-Events erzeugen falsche Population.

**PASS:** Population ist deterministisch nachvollziehbar und DB-/Session-State konsistent.

---

# E. Bewegung / Pathfinding

## E1. Normale Bewegung

- [ ] kurze Wege.
- [ ] lange Wege.
- [ ] Innenräume.
- [ ] Außenwelt.
- [ ] Höhenunterschiede.
- [ ] Brücken.
- [ ] Rampen.
- [ ] Treppen.
- [ ] Engstellen.

## E2. MMaps / VMAPs

- [ ] keine häufigen Move-Fails auf gültigem Terrain.
- [ ] keine unnötigen Relocations.
- [ ] keine Bots unter Terrain.
- [ ] keine Bewegung durch Wände.

## E3. Fall-State

- [ ] Fall-Erkennung korrekt.
- [ ] AI blockiert nicht dauerhaft nach Fall.
- [ ] normaler Bewegungszustand wird wiederhergestellt.

**PASS:** kein wiederholtes Ziel-Anlaufen ohne sinnvolle Aktion bei normal erreichbaren Zielen.

---

# F. Transport / Travel

## F1. Boote / Zeppeline

Für mehrere echte Routen:

- [ ] Bot läuft zum Hafen / Zeppelin-Turm.
- [ ] wartet am korrekten Punkt.
- [ ] erkennt das echte Transportobjekt.
- [ ] läuft physisch auf den Transport.
- [ ] bleibt korrekt Passenger.
- [ ] fährt mit.
- [ ] Map-/Segmentwechsel funktioniert.
- [ ] läuft physisch vom Transport herunter.
- [ ] setzt Reise danach fort.

**Verbotene Normalpfade prüfen:**

- [ ] kein `TeleportTo()` aufs Schiff.
- [ ] kein Beam zum Zielhafen.
- [ ] kein „Transport fehlt → Warp“.
- [ ] kein Random-Recovery-Teleport.

## F2. Elevator / Tram / statische Transporte

- [ ] Transport wird über vollständigen GameObject-GUID gefunden.
- [ ] Elevator erkannt.
- [ ] Tram erkannt, soweit im Content vorhanden.
- [ ] kein invalid-GUID Lookup.
- [ ] kein falscher `HIGHGUID_TRANSPORT`/`HIGHGUID_GAMEOBJECT`.

## F3. Travel Graph

- [ ] kontinentaler Travel.
- [ ] Zonenwechsel.
- [ ] MapTransfers.
- [ ] Taxi.
- [ ] Portale als echte Spielmechanik.
- [ ] Travel-Destinations entstehen aus allen vier nativen Quest-Relation-Maps (Creature giver/taker, GO giver/taker).
- [ ] keine falsche Verkürzung durch Playerbot-Teleports.

**PASS:** Reise funktioniert spielmechanisch, nicht per Star-Trek-Modus. 😄

---

# G. GameObjects / Loot

## G1. Normale Kisten / Food Crates

- [ ] Bot erkennt Kiste.
- [ ] läuft hin.
- [ ] richtet sich korrekt aus.
- [ ] `OPEN_LOCK` auf echtes `GameObject`.
- [ ] öffnet.
- [ ] lootet.
- [ ] entfernt Ziel aus Loot-State.
- [ ] läuft danach weiter.
- [ ] keine Hin-/Weg-/Hin-Endlosschleife.

## G2. Quest-GameObjects

- [ ] Quest-Kisten.
- [ ] Quest-Gegenstände.
- [ ] klickbare Questobjekte.
- [ ] Türen / Goober.
- [ ] keine falsche Loot-Klassifikation.

## G3. Loot Manager / natives Penqle Loot

- [ ] Creature-Loot wird über `Creature::loot` gelesen.
- [ ] GameObject-Loot über `GameObject::loot`.
- [ ] `sLootMgr`-Adapter liefert echten Loot.
- [ ] Gold wird erkannt.
- [ ] Itemlisten werden erkannt.
- [ ] Bagspace-Prüfung funktioniert.
- [ ] Loot-Release korrekt.
- [ ] `playersLooting` korrekt.
- [ ] Corpse acceleration/despawn korrekt.

**PASS:** kein `nullptr`-Stub beeinflusst echtes Lootverhalten.

---

# H. Gathering / Skills

## H1. Mining

- [ ] Mine erkannt.
- [ ] Bot hat Mining → benutzt Node.
- [ ] Spell target ist GameObject.
- [ ] Skill-Anforderung korrekt.
- [ ] Loot danach korrekt.

## H2. Herbalism

- [ ] Herb erkannt.
- [ ] Bot hat Herbalism → benutzt Herb.
- [ ] Spell target ist GameObject.
- [ ] Quest-Herbs korrekt unterschieden.
- [ ] Loot danach korrekt.

## H3. Skinning

- [ ] Creature-Skinning.
- [ ] korrekter Skillcheck.
- [ ] korrektes Unit-Ziel.
- [ ] kein GameObject-Pfad.

## H4. Lock-Spell Effektprüfung

- [ ] OPEN_LOCK in Effect 0.
- [ ] OPEN_LOCK in Effect 1.
- [ ] OPEN_LOCK in Effect 2.
- [ ] SKINNING-Effekt entsprechend.
- [ ] nicht passende Effekte verhindern nicht voreilig die Prüfung der späteren Slots.

**PASS:** alle relevanten Spell-Effect-Slots werden berücksichtigt.

---

# I. Quests / Leveling

## I1. Questannahme

- [ ] Questgeber finden.
- [ ] Creature-Questgiver aus nativer `ObjectMgr`-Questrelation finden.
- [ ] GameObject-Questgiver aus nativer `ObjectMgr`-Questrelation finden.
- [ ] negative GO-Entry-Konvention (`-entry`) bleibt bei Questzielen/-relationen korrekt.
- [ ] Levelanforderung.
- [ ] Race/Class-Anforderung.
- [ ] Faction.
- [ ] Vorquests.
- [ ] Folgequests.
- [ ] Breadcrumbs ohne Holzhammer-Blacklist.

## I2. Questziele

- [ ] Kill quests.
- [ ] Loot quests.
- [ ] GameObject quests.
- [ ] Use-item quests.
- [ ] Escort soweit unterstützt.
- [ ] Explore / Area objectives.
- [ ] Multi-zone objectives.

## I3. Turn-in

- [ ] korrekten Questgeber finden.
- [ ] Creature-Involved-/QuestTaker-Relation wird vollständig gefunden.
- [ ] GameObject-Involved-/QuestTaker-Relation wird vollständig gefunden.
- [ ] Questgiver/QuestTaker-Factionfilter verwendet Penqles `CreatureInfo::faction`.
- [ ] richtige Area/Position.
- [ ] Reward auswählen.
- [ ] Quest abschließen.
- [ ] Folgequest aufnehmen.

## I4. Level-Fortschritt

- [ ] Level 1 → mehrere Level ohne manuellen Eingriff.
- [ ] Gebietswechsel passend zum Level.
- [ ] keine dauerhafte Schleife in einer Zone.
- [ ] keine Kohorten, die reproduzierbar in Arathi hängen.
- [ ] kein zufälliger Teleport als Progress-Fix.
- [ ] `DisableRandomLevels = 1` respektiert.

**PASS:** Bots steigen durch echtes Spielen auf.

---

# J. Trainer / Skills / Equipment / Economy

- [ ] Trainer finden.
- [ ] neue Spells lernen.
- [ ] Klassen-/Rassenfilter korrekt.
- [ ] Skilltraining.
- [ ] Reparieren.
- [ ] Vendor.
- [ ] verkaufen.
- [ ] kaufen.
- [ ] Geldmanagement.
- [ ] Equip-Upgrades erkennen.
- [ ] Equip korrekt anlegen.
- [ ] Bags korrekt verwalten.
- [ ] fehlgeschlagene Bag-Swaps erzeugen keine Endlosschleife.

**PASS:** ein Bot bleibt langfristig spiel- und handlungsfähig.

---

# K. Combat AI

## K1. Basiskampf

- [ ] Melee.
- [ ] Ranged.
- [ ] Caster.
- [ ] Heiler.
- [ ] Tank.
- [ ] Pet-Klassen.

## K2. NPC-Typ-Erkennung

- [ ] ranged NPC korrekt erkannt.
- [ ] melee NPC korrekt erkannt.
- [ ] scripted-death / preventing-death Mechanik korrekt erkannt.
- [ ] keine falschen Compatibility-Stubs beeinflussen Entscheidungen.

## K3. Ressourcen

- [ ] Mana.
- [ ] Rage.
- [ ] Energy.
- [ ] Ammo falls relevant.
- [ ] Food/Drink.
- [ ] Potions/Bandages soweit unterstützt.

**PASS:** keine offensichtlichen API-Portfehler in Kampfentscheidungen.

---

# L. Gruppe / Party / Raid

- [ ] Invite.
- [ ] Join.
- [ ] Leave.
- [ ] Leader.
- [ ] Party-Kommandos wirken nur auf richtigen Scope.
- [ ] Raid-Kommandos wirken nur auf richtigen Scope.
- [ ] Formation.
- [ ] Follow.
- [ ] Tank/Heal/DPS Rollen.
- [ ] Loot in Gruppe.
- [ ] Group loot / rolls soweit unterstützt.
- [ ] keine fremden Gruppen beeinflusst.

**PASS:** Gruppenlogik bleibt isoliert und stabil.

---

# M. Battleground / PvP

## M1. Queue

- [ ] mehrere Bots können gleichzeitig queue-relevante Daten lesen/schreiben.
- [ ] kein Race / Iterator-Crash.
- [ ] Join/Leave.
- [ ] Queue Count korrekt.

## M2. Battleground Semantik

- [ ] Flag-Carrier-Erkennung.
- [ ] Flag Base.
- [ ] Team/Faction.
- [ ] AV nodes.
- [ ] contested / occupied states.
- [ ] keine Default-GUID-Platzhalter mehr im aktiven Pfad.

## M3. PvP AI

- [ ] Zielpriorisierung.
- [ ] Heiler priorisieren.
- [ ] Focus.
- [ ] CC.
- [ ] Defensive Fähigkeiten.
- [ ] Retreat / regroup.
- [ ] Objective > sinnloser Einzelkampf.

**PASS:** funktional korrekt; Intelligenzverbesserung wird separat bewertet.

---

# N. Tod / Geist / Wiederbelebung

- [ ] normaler Tod.
- [ ] Corpse Run.
- [ ] Spirit Healer.
- [ ] Resurrection.
- [ ] Death-Historie bleibt lange genug erhalten.
- [ ] Deadloop wird erkannt.
- [ ] keine Endlosschleife.
- [ ] kein Recovery-Beam im Defaultbetrieb.
- [ ] State nach Revive vollständig sauber.

**PASS:** Tod zerstört weder AI-State noch Fortschritt.

---

# O. Inventory / Items / Bags

- [ ] volle Bags.
- [ ] teilweise volle Stacks.
- [ ] Stack-Merge.
- [ ] Swap.
- [ ] Equip.
- [ ] Unequip.
- [ ] Questitems.
- [ ] Soulbound.
- [ ] Consumables.
- [ ] fehlgeschlagene Operationen haben begrenzte Retries.
- [ ] kein CPU-Loop.

**PASS:** kein Item-Fehler kann die AI dauerhaft blockieren.

---

# P. Auction House Bot

Nur falls aktiviert/geportet:

- [ ] alle verwendeten AH-Core-APIs sind echte Penqle-APIs.
- [ ] keine Fake-Defaults beeinflussen Preise/Entscheidungen.
- [ ] Kaufen.
- [ ] Verkaufen.
- [ ] Auktion erstellen.
- [ ] Auktion auslaufen.
- [ ] Mail-Ergebnis.
- [ ] kein Dupe.
- [ ] kein Goldverlust durch Portfehler.

**PASS:** sonst AHBot deaktiviert lassen.

---

# Q. Strategy / Engine Lifecycle

- [ ] Strategy hinzufügen.
- [ ] Strategy entfernen.
- [ ] mehrere Änderungen in einem Tick.
- [ ] Engine wird höchstens einmal am Tick-Ende rebuilt.
- [ ] keine Container-/Iterator-Invalidierung.
- [ ] kein Action-State-Verlust mitten im Execute.
- [ ] CustomStrategy Negative Cache korrekt invalidiert.

**PASS:** Strategy-Wechsel erzeugt keine Rebuild-Stürme.

---

# R. Concurrency / Thread Safety

- [ ] Map-local Transport-Set unter parallelem Map Update.
- [ ] Add/Remove/GetTransports parallel.
- [ ] BG queue locking.
- [ ] ChannelBroadcaster.
- [ ] Bot login/logout unter Last.
- [ ] Looting mehrerer Bots gleichzeitig.
- [ ] Gruppenoperationen parallel.
- [ ] keine TSAN-artigen Symptome: sporadische Crashes, corrupted sets, invalid iterators.

**PASS:** längerer Lastlauf ohne sporadischen Concurrency-Fehler.

---

# S. Performance / Skalierung

Mit kleiner, mittlerer und Zielpopulation:

- [ ] 50 Bots.
- [ ] 150 Bots.
- [ ] 300 Bots.
- [ ] 648 Bots.

Messen:

- [ ] mangosd CPU.
- [ ] Thread-Auslastung.
- [ ] RAM.
- [ ] DB Queries/s.
- [ ] Tick-/Update-Zeiten.
- [ ] Playerbot AI Update-Zeit.
- [ ] Travel-Initialisierung.
- [ ] Login-Burst.
- [ ] Logout-Burst.
- [ ] RandomBot DB Hotpaths.
- [ ] Index `ai_playerbot_random_bots(owner, bot, event)` falls übernommen.

**PASS:** keine exponentielle Verschlechterung / kein einzelner Hotloop dominiert.

---

# T. Long-Run / Soak Test

- [ ] 1 Stunde.
- [ ] mehrere Stunden.
- [ ] über Nacht.
- [ ] mehrere Server-Restarts im Testzeitraum.

Beobachten:

- [ ] Onlinezahl stabil.
- [ ] Levelverteilung bewegt sich.
- [ ] Bots verteilen sich plausibel über Welt.
- [ ] keine Zone sammelt unnatürlich große Bot-Kohorte.
- [ ] keine steigende Memory-Kurve.
- [ ] keine ständig wachsenden DB-Events.
- [ ] keine zunehmenden Loot-/Travel-/Death-Loops.
- [ ] keine Session-Leaks.

**PASS:** System bleibt über lange Laufzeit stabil.

---

# U. LivingBots-spezifische Endabnahme

Wenn der eigene LivingBots-Layer eingebaut ist:

- [ ] Prefix `LivingBots_`.
- [ ] erwartete Accounts.
- [ ] erwartete Charakterzahl.
- [ ] dauerhafte Identität.
- [ ] persistente Level.
- [ ] persistentes Equipment.
- [ ] persistente Skills.
- [ ] persistente Position, soweit gewollt.
- [ ] normale Weltaktivität.
- [ ] kein klassisches RandomBot-Neuwürfeln.
- [ ] keine zufälligen Level.
- [ ] keine Default-Teleport-Recovery.
- [ ] Population nach Restart korrekt rekonstruiert.

**PASS:** Bots wirken wie dauerhafte Spielerpopulation und nicht wie wegwerfbare RandomBots.

---

# V. Abschlusskriterien

Der Clean-Port gilt erst als testseitig bestanden, wenn:

- [ ] Build sauber.
- [ ] Serverstart sauber.
- [ ] Config sauber.
- [ ] Rassen/Klassen sauber.
- [ ] Transport physisch.
- [ ] Questing/Leveling funktioniert.
- [ ] Loot/Gathering funktioniert.
- [ ] Population konsistent.
- [ ] Death/Inventory ohne Loops.
- [ ] Gruppen/BG stabil.
- [ ] keine Default-Recovery-Teleports.
- [ ] 648-Bot-Last vertretbar.
- [ ] Soak-Test ohne State-/Memory-/DB-Leak.
- [ ] alle Abweichungen mit Commit-SHA + Logauszug dokumentiert.
