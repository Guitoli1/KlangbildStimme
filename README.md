# Klangbild Stimme

Gesangskanal mit De-Esser im blaugrauen Klangbild-Rack-Look – AU, VST3 und Standalone für macOS (Logic Pro).

## Signalweg

```
TRITTSCHALLFILTER → DE-ESSER → KOMPRESSOR → KLANG (Wärme · Präsenz · Luft) → RÖHRENWÄRME → AUSGANG
```

| Abschnitt | Aufgabe |
|---|---|
| **Trittschall** | Hochpass 40–300 Hz gegen Rumpeln, Ständergeräusche und Atemstöße |
| **De-Esser** | Senkt nur das Zischband oberhalb der Frequenz (3–10 kHz) ab – und nur, wenn es im Verhältnis zur Stimme zu laut ist. Dadurch arbeitet er bei leisen und lauten Passagen gleich. Die LED-Kette zeigt die Absenkung. |
| **De-Esser abhören** | Nur das, was abgesenkt wird – so findest du die richtige Frequenz |
| **Kompressor** | Weicher 3:1-Kompressor nach dem De-Esser (Zischlaute lösen ihn nicht aus); VU zeigt die Pegelreduktion |
| **Wärme** | Glocke bei 250 Hz – Körper oder weniger Mulm |
| **Präsenz** | Glocke bei 4,5 kHz – Verständlichkeit, nach vorne |
| **Luft** | Höhen-Shelf bei 12 kHz – Glanz |
| **Röhrenwärme** | Sanfte, leicht unsymmetrische Sättigung |

## Presets

| Preset | Gedacht für |
|---|---|
| Standard | Neutraler Startpunkt |
| Soul-Gesang – warm | Warm, dicht, mit Glanz |
| Blues – rau mit Biss | Mehr Präsenz und Kompression |
| Ballade – nah & offen | Nah, offen, viel Luft |
| Rock – nach vorne | Stark komprimiert, präsent |
| Background-Chor | Schlanker, sitzt hinter der Leadstimme |
| Scharfe Stimme zähmen | Kräftiger De-Esser, weniger Präsenz |
| Sprache / Ansage | Gleichmäßig und verständlich |
| Mundharmonika | Für Blues-Harp: warm, ohne De-Esser, Höhen gedämpft |

Alle Presets sind auf etwa gleiche Lautstärke eingepegelt.

## Installation

Über den [Klangbild Installer](https://github.com/Guitoli1/KlangbildInstaller):

```
curl -fsSL https://raw.githubusercontent.com/Guitoli1/KlangbildInstaller/main/install.sh | bash
```

## Build

JUCE 8 / CMake. GitHub Actions baut auf macOS 15 ein Universal Binary (arm64 + x86_64), signiert ad-hoc und prüft mit `auval`.
