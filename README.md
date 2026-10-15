# FM-1 Unholy Union generative drone firmware

## Press PLAY to pray

UnholyUnion is my generative drone firmware for the M-VAVE FM-1. **Immaculate Permutation** rolls an ambient arrangement in a shared key, with a pad, sequence and chord arpeggio beneath a fourth voice for your hands. Keep a sermon you like, evolve it while playing, or reveal another.

[Enter the Ritual Room — installer and guides](https://actuallysparky.github.io/FM-1-Unholy-Union-generative-drone-firmware/) · [Download compiled firmware — 0.2.3](https://github.com/actuallysparky/FM-1-Unholy-Union-generative-drone-firmware/releases/download/v0.2.3/unholyunion-permutation-0.2.3.fwsc) · [Illustrated quick start (PDF)](https://actuallysparky.github.io/FM-1-Unholy-Union-generative-drone-firmware/UnholyUnion-Quick-Start-0.2.3.pdf) · [Release and guides](https://github.com/actuallysparky/FM-1-Unholy-Union-generative-drone-firmware/releases/tag/v0.2.3)

## What it does

- A curated first-run project, **Evening Vow**, ready when you press PLAY.
- Ambient-biased generation with warm presets, spacious rhythms and slower tempos. Choose Minor, Dorian, Minor pentatonic or Major pentatonic.
- Seamless harmony evolution with ENV + OCT+, and gentle four-track modulation with LFO + OCT+.
- Automatic evolution every 2, 4, 8, 12, 16, 24 or 32 bars; 16 bars is the fresh-start default.
- Live **sermon checkpoints**: hold SAVE for 1.2 seconds, wait for SESSION SAVED, then load the named project from A–D. Hold OCT- to undo the last rite.
- The **Choir Loft** editor and **Altar of Four Winds** macro desk, backups and project exports.
- **GILT SIN**: gold, silver, blood crimson and black. Good taste has left the chapel.

On ordinary performance pages, OCT+/OCT- transpose the selected track's keyboard, arp and sequence. Immaculate Permutation keeps OCT+ for Reveal.

## Receive the offering

Open the [Ritual Room](https://actuallysparky.github.io/FM-1-Unholy-Union-generative-drone-firmware/) in desktop Chrome or Edge, connect one FM-1, optionally **Gather the scriptures**, then press **Install Apocryphal**. The page uses USB MIDI/SysEx over HTTPS.

Installation preserves saved projects, presets, samples and settings. The instrument's Return to Evening Vow reset restores the current session while keeping saved projects and presets.

This offering is **0.2.3**, identity FM-1_80023. I've confirmed sermon saving on my FM-1 without audible glitches. Back up your work before installing; saves from this fork cannot be read by older Felucca or Jangada firmware.

The release includes the firmware, corresponding source and SDK data, the Book of Sacred Sounds and checksums. Keep USB and power connected throughout installation.

## Praise the saints

**Felucca**, by Leo Kuroshita (@kurogedelic), Hügelton Instruments and its contributors, supplies the instrument's foundation: engines, sequencing, modulation, USB/MIDI, storage, updater and editor. [Visit Saint Felucca](https://github.com/hugelton/Felucca).

**Jangada**, by zednaked and its contributors, supplies the drone algorithm adapted for evolving sustain and tension. [Visit Saint Jangada](https://github.com/zednaked/jangada).

**Cyma Forma and Bambounou's RND synth** inspired the one-button invitation. Immaculate Permutation is our own ambient, editable interpretation; no RND code or assets were copied. [Visit the archangel of chance](https://www.cymaforma.com/rnd-synth).

This is an unaffiliated GPL-3.0-only derivative. Upstream copyrights, licenses and asset notices remain in the tree. See [LICENSING.md](LICENSING.md), [provenance.json](provenance.json) and the retained [Felucca README](UPSTREAM_README.md). You can support the upstream saints through the retained funding links.

## Build and contribute

See [BUILDING.md](BUILDING.md) for the Linux compiler and SDK-data setup, and [CONTRIBUTING.md](CONTRIBUTING.md) for tests and bug reports.
