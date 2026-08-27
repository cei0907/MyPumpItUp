# PumpDX Rebuild — Implementation Plan v0.1

This document turns the [core design](DESIGN.md) into ordered deliverables and completion gates. The legacy `DxPumpV0.76` project is not modified or deleted.

## Current status

- [x] Legacy code, chart, and input approach reviewed
- [x] Core design fixed: timing, charts, holds, scenes, BGA, two-player input, themes
- [x] Object/file layout and theme-package design defined
- [x] Runtime technology selected: C++20, CMake, Direct3D 11, followed by FMOD, Media Foundation, and Raw Input/DirectInput
- [x] New executable project skeleton created
- [x] CMake/Visual Studio C++ toolchain installed and first build verified

### State work-unit tracking

The `State NN` prefix is a small, reviewable Git work unit inside the numbered implementation stages below; it is not a replacement for those higher-level stages.

- [x] State 02 — Framework/game boundary, logical viewport, empty scene flow, and timing-map base
- [x] State 03 — Play-session/result-data handoff and Debug runtime alignment
- [x] State 04 — 1280×720 Direct2D/DirectWrite scene overlay for visible scene-flow verification
- [x] State 05 — Default theme manifest, palette application, and safe semantic resource resolution
- [x] State 06 — Immutable chart model, five-panel note events, and explicit hold tick counts
- [x] Stage 02 / State 07 — Debug song clock, chart-time note projection, five-lane field, and keyboard panel state
- [x] Stage 02 / State 08 — Time-error judgement, automatic misses, score/combo state, gameplay HUD, and boundary tests
- [x] Stage 02 / State 09 — FMOD AudioClock, local-only music path, and external song manifest
- [x] Stage 02 / State 10 — Legacy `.stp` tap-chart import, energy state, automatic song end, and result summary
- [x] Stage 02 / State 11 — Static BGA image/fallback, visible energy gauge, and vertical-slice completion review
- [x] Stage 03 / State 12 — Readable `.pdxchart` source loader, exact tuplets, tempo changes, and hold tick metadata
- [x] Stage 03 / State 13 — Hold head/tick/end runtime judgement, combo/gauge scoring, and PIU-style re-hold tests
- [x] Stage 03 / State 14 — NewSongToGod native hold playtest chart and active-hold field feedback
- [x] Stage 03 / State 15 — Same-lane tap/hold collision validation and conflict-free legacy hold overlay
- [x] Stage 03 / State 16 — PIU-inspired 1P field/HUD layout, judgement/combo feedback, and hold-state visuals
- [x] Stage 03 / State 16-1 — Consumed-hold body clipping, release continuation, and successful-end cleanup
- [x] Stage 03 / State 16-2 — PIU-reference 1P top-receptor HUD repositioning

## Phase 0 — Technology and development foundation

**Purpose:** Establish the toolchain, build, and test foundation needed for executable code.

**Decision:** renderer, audio, BGA video, USB input, build system, and test framework.

The selected stack is Windows-native **C++20 + Direct3D 11 + FMOD + Media Foundation + Raw Input/DirectInput + CMake**. It modernises the original C++/DirectX experience while retaining direct control of USB pads and BGA video.

**Completion gate:** an empty window can open/close, development and release builds work, and a minimal automated test runs. **Status: completed on 2026-08-27.**

## Phase 1 — Runtime skeleton and data boundaries

**Purpose:** Implement the smallest game loop, resource, scene, and chart boundaries so later features do not mix together.

- `GameApplication`, `SceneManager`, `ResourceCache`, and a 1280×720 logical viewport
- immutable `Chart`, `TimingMap`, `PlaySession`, and `ResultData` models
- empty `MainMenu → SongSelect → Gameplay → Result` transitions
- a base theme manifest and asset-key lookup

**Completion gate:** a dummy song/chart can transition through every scene without scene code hard-coding asset paths or game state.

**Status: completed through Stage 02 / State 11. Phase 2 completion gate passed with the local `NewSongToGod` integration.**

## Phase 2 — First playable vertical slice

**Purpose:** Play one complete song using accurate audio time.

- `AudioClock` and music playback (the current debug clock is an intentional temporary adapter)
- tap-chart loading, active note views, scroll-speed control (State 10 imports `NewSongToGod_10.stp` without modifying it)
- keyboard input, time-error judgement, automatic misses
- combo, score, basic life gauge, and result scene
- static BGA fallback
- beat-to-time and judgement-window tests

**Completion gate:** scroll speed changes visual distance but not judgement windows, and one song produces an accurate result.

**Status: completed on the local integration build.** The first playable vertical slice now uses a local FMOD song clock, imported legacy tap chart, time-based judgement, score/combo/energy, static BGA with a safe fallback, and a result handoff. Long-note rules, animated judgement effects, and video BGA stay in Phase 3.

## Phase 3 — Pump-style gameplay and presentation

**Purpose:** Complete basic play with holds, gauge rules, and scene motion.

- hold head/tick/end and variable `tickPolicy`
- ruleset-driven life, stage failure, and grade
- pooled judgement/combo/receptor/pad/gauge effects
- 1280×720 UI layout and `SceneTimeline`
- song BGA video, dropped-frame handling, and static fallback

**Completion gate:** equal-length holds may use different combo counts, and BGA, notes, and judgement remain synchronised to the same audio clock.

## Phase 4 — Chart editor and content migration

**Purpose:** Author and verify real songs without a fixed 16th-note table.

- separate browser Chart Editor
- arbitrary N-grid, triplets, BPM/time signatures, taps, drag-created holds
- audio preview/waveform, undo/redo, copy/paste, validation
- source chart save and compiled runtime chart output
- legacy `.stp` tap-chart import

**Completion gate:** a chart with holds and triplets created in the editor compiles and plays at matching times in the game.

## Phase 5 — Skin and theme tools

**Purpose:** Replace and preview game imagery without code changes.

- `ThemeManifest` validation and default/user-theme switching
- image-slot replacement, pivot/crop/tint/opacity editing
- sprite-sheet and UI/effect animation definitions
- scene preview plus theme duplicate/import/export

**Completion gate:** a new theme package can change menu, gameplay, and result assets without changing judgement geometry or game rules.

## Phase 6 — USB pads and two-player play

**Purpose:** Assign two identical USB pads safely to P1/P2 and support simultaneous play.

- independent controller-specific `PadDevice`s
- device assignment, input testing, remapping, and input offset
- reconnect handling and keyboard fallback
- independent P1/P2 judgement, score, gauge, result HUD
- versus and co-op session rules

**Completion gate:** two identical pads are assigned by centre-panel press and two players complete a song with independent results.

## Phase 7 — Quality and release preparation

**Purpose:** Produce a stable portfolio- and user-ready version.

- saved settings; fullscreen; display, audio, and reduced-motion options
- loading, error, and missing-resource handling
- expanded automated tests and long-play testing
- user/creator docs, example themes/charts, build guide
- play footage and before/after portfolio material

**Completion gate:** a fresh environment can build, run, test, and add a song by following the documentation alone.

## Execution rules

1. Verify a phase completion gate before entering the next phase.
2. Record new feature ideas in the closest phase and protect the core vertical slice from priority drift.
3. Update relevant design, test, and user documents with each implementation change.
4. Confirm the scope before introducing large external dependencies, moving/deleting existing files, or publishing a build.

## GitHub milestone workflow

`PumpDX_Rebuild` is managed as a Git repository separate from the legacy folder. This workspace does not currently contain a detectable Git repository or configured remote, so Phase 0 initialises the repository and connects the GitHub remote specified by the user. Confirm the remote URL and public/private visibility before connecting.

- Keep work in small, readable commits whose messages explain the purpose of the change.
- When a phase completion gate passes, merge it to `main` and create a tag and GitHub Release.
- Recommended first tags are `v0.0.1-foundation` and, for the first playable build, `v0.1.0-vertical-slice`; subsequent major phases receive meaningful version increments.
- A release records changes, verification steps, known limits, and links to builds, video, or screenshots.
- Version source, documentation, tests, and small example charts/themes in Git.
- Keep build artifacts, IDE caches, logs, and generated files in `.gitignore`. Choose Git LFS or a separate distribution method for large music/BGA assets only after checking size and licensing.
- Do not copy legacy `DxPumpV0.76` binaries/intermediate output into the new repository. Migrate only intentional data and assets whose permissions are clear, in separate commits.
