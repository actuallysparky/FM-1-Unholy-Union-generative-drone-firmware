# FM-1 Unholy Union generative drone firmware

## Press PLAY to pray

UnholyUnion is my generative drone firmware for the M-VAVE FM-1. **Immaculate Permutation** rolls an ambient arrangement in a shared key, with a pad, sequence and chord arpeggio beneath a fourth voice for your hands. Keep a sermon you like, evolve it while playing, or reveal another.

[Enter the Ritual Room — installer and guides](https://actuallysparky.github.io/FM-1-Unholy-Union-generative-drone-firmware/) · [Download compiled firmware — 0.2.4](https://actuallysparky.github.io/FM-1-Unholy-Union-generative-drone-firmware/unholyunion-permutation-0.2.4.fwsc) · [Illustrated quick start (PDF)](https://actuallysparky.github.io/FM-1-Unholy-Union-generative-drone-firmware/UnholyUnion-Quick-Start-0.2.3.pdf) · [Previous release and guides](https://github.com/actuallysparky/FM-1-Unholy-Union-generative-drone-firmware/releases/tag/v0.2.3)

## What it does

- A curated first-run project, **Evening Vow**, ready when you press PLAY.
- Ambient-biased generation with warm presets, spacious rhythms and slower tempos. Choose Minor, Dorian, Minor pentatonic or Major pentatonic.
- Seamless harmony evolution with ENV + OCT+, and gentle four-track modulation with LFO + OCT+.
- Automatic evolution every 2, 4, 8, 12, 16, 24 or 32 bars; 16 bars is the fresh-start default.
- Live **sermon checkpoints**: hold SAVE for 1.2 seconds, wait for SESSION SAVED, then load the named project from A–D. Hold OCT- to undo the last rite.
- The **Choir Loft** editor and **Altar of Four Winds** macro desk, backups and project exports.
- **GILT SIN**: gold, silver, blood crimson and black. Good taste has left the chapel.

On ordinary performance pages, OCT+/OCT- transpose the selected track's keyboard, arp and sequence. Immaculate Permutation keeps OCT+ for Reveal. **Tap OCT− for the full instrument editor** without stopping the music; **hold OCT− to undo** the last rite.

## Receive the offering

Open the [Ritual Room](https://actuallysparky.github.io/FM-1-Unholy-Union-generative-drone-firmware/) in desktop Chrome or Edge, connect one FM-1, optionally **Gather the scriptures**, then press **Install Apocryphal**. The page uses USB MIDI/SysEx over HTTPS.

The installer accepts stock V14 (`FM-1_014`) and V15 (`FM-1_015`). V15-to-UnholyUnion 0.2.3 installation has been confirmed on one instrument. Stock presets and sequences are not converted; complete backups through this installer are available for UnholyUnion, not stock firmware.

When upgrading UnholyUnion, installation preserves saved projects, presets, samples and settings. The instrument's Return to Evening Vow reset restores the current session while keeping saved projects and presets.

This Experimental offering is **0.2.4**, identity FM-1_80024. The boot greeting and launch screen now explicitly show how to reach the full instrument editor: tap OCT−. Hold OCT− still undoes the last rite. The navigation change passes native and reproducible target-build checks; an instrument trial remains pending. I've confirmed sermon saving on my FM-1 without audible glitches. Back up your work before installing; saves from this fork cannot be read by older Felucca or Jangada firmware.

The Ritual Room includes the current firmware and corresponding source/SDK data. The linked 0.2.3 guides still cover the controls; 0.2.4 makes the editor exit explicit. Keep USB and power connected throughout installation.

## Praise the saints

**Felucca**, by Leo Kuroshita (@kurogedelic), Hügelton Instruments and its contributors, supplies the instrument's foundation: engines, sequencing, modulation, USB/MIDI, storage, updater and editor. [Visit Saint Felucca](https://github.com/hugelton/Felucca).

**Jangada**, by zednaked and its contributors, supplies the drone algorithm adapted for evolving sustain and tension. [Visit Saint Jangada](https://github.com/zednaked/jangada).

**Cyma Forma and Bambounou's RND synth** inspired the one-button invitation. Immaculate Permutation is our own ambient, editable interpretation; no RND code or assets were copied. [Visit the archangel of chance](https://www.cymaforma.com/rnd-synth).

This is an unaffiliated GPL-3.0-only derivative. Upstream copyrights, licenses and asset notices remain in the tree. See [LICENSING.md](LICENSING.md), [provenance.json](provenance.json) and the retained [Felucca README](UPSTREAM_README.md). You can support the upstream saints through the retained funding links.

## Build and contribute

See [BUILDING.md](BUILDING.md) for the Linux compiler and SDK-data setup, and [CONTRIBUTING.md](CONTRIBUTING.md) for tests and bug reports.
