# PumpDX Rebuild — Core Design v0.1

## 1. Purpose and boundaries

This is a clean, single codebase rebuild of the 2019 `DxPumpV0.76` project.  The original project remains untouched as a legacy reference and as a source of assets/data that may later be migrated deliberately.

The goal is a Pump It Up-inspired five-panel rhythm game with accurate music-synchronised play, song BGA video, long notes, a browser-based chart editor, and one- and two-player USB pad support.

The rebuild must not copy across experimental or unrelated legacy systems.  Only a defined runtime, chart format, editor, and reusable assets may enter this project.

## 2. Product principles

1. **Audio is the source of truth.** Gameplay timing never comes from frame count or rendered Y coordinates.
2. **Chart data is musical data.** A chart represents beats and events, not fixed rows of pixels or fixed 16th-note strings.
3. **Rendering is derived.** Note position, BGA frame, effects, and animation are derived from the current audio time.
4. **Input belongs to a device and a player.** Multiple USB pads are never merged into one global button array.
5. **Runtime and authoring are separate.** The editor produces readable source charts; the game reads a validated compiled chart.
6. **1280×720 is the design canvas, not a window-size restriction.** Existing 16:9 assets remain usable while the game scales to other display sizes.

## 3. Timing model

### 3.1 Master clock

`AudioClock` exposes the audio playback position in seconds.  Every gameplay frame samples it once:

```text
songTime = audioPlaybackTime + perPlayerInputOffset
```

`songTime` drives all of the following:

- input judgement;
- automatic misses;
- note and hold-body positions;
- hold-tick timing;
- BGA video selection;
- beat-driven visual effects.

The game never advances notes by accumulating a per-frame Y position.  Instead, it calculates their current display position from `noteTime - songTime`.  A dropped frame may skip a visual position, but the next frame is immediately correct and gameplay timing does not drift.

### 3.2 Musical time

The source chart stores beat positions as exact rational values, not binary floating-point values.  For example, a triplet can be written as `17 + 1/3` and an arbitrary tuplet as a fraction.

`TimingMap` converts these beat positions to seconds.  It supports, from the first format version:

- BPM changes;
- time-signature changes for editor display;
- optional stops/pauses;
- a chart offset relative to the audio start.

## 4. Chart model

### 4.1 Song package

A song package contains:

- audio;
- an optional BGA video and a static fallback image;
- title/banner/preview assets;
- song metadata;
- one or more difficulty charts.

### 4.2 Note events

Every chart uses five lanes: `SW`, `NW`, `Center`, `NE`, and `SE`.

```text
Tap  { lane, beat }
Hold { lane, startBeat, endBeat, tickPolicy }
```

A hold is one authoring event, even though its renderer and scoring system create several internal events.  It is not stored as a long sequence of body cells.

State 06 implements this immutable `Chart` contract with five lanes, `TapNote`, `HoldNote`, ordering, and validation.  The first executable `tickPolicy` mode is an explicit fixed count, so two holds with the same start/end beats can intentionally request 10 and 100 ticks.  Beat-interval policies are added later by the chart compiler without changing the hold event shape.

`tickPolicy` explicitly controls scoring density. The first executable policy is a requested fixed `tickCount`; the runtime resolves it into evenly distributed sustain points. Therefore two holds with the same visual length can intentionally produce 10 or 100 sustain combo events.

For version 1 of the gameplay rules:

- the hold head is judged like a tap;
- a successful head enables the hold, but a body press may also catch an already-passing hold;
- every generated sustain point checks whether that lane is still held;
- early release loses only the sustain points that pass while the lane is up; pressing again resumes at the next available point;
- the final sustain point always occurs at the hold end;
- the visual body always spans `startBeat` to `endBeat`, independently of tick count.

The fixed `tickCount` is the number of sustain points after the head; the last one is the end point. Therefore a completed `hold=...,10` yields one head judgement plus ten sustain combo events. This keeps hold duration, combo density, and endpoint handling explicit. Re-hold behaviour is a gameplay ruleset decision and never changes rendering behaviour.

The validator rejects malformed holds (`endBeat <= startBeat`), overlapping holds on one lane, and every tap whose beat falls on or between the start and end of a hold on that same lane. Different lanes remain independent, so simultaneous holds or taps on other panels are valid.

### 4.3 Compiled chart

The browser editor saves a readable source chart.  A compiler validates it and creates a runtime chart whose events are sorted by audio time, carry resolved hold ticks, and can be loaded without runtime parsing work.

The former `.stp` files can be imported as fixed 16th-grid tap charts.  This is a migration path only; the new format does not inherit that limitation.

### 4.4 Native source chart (`.pdxchart`)

State 12 introduces the first readable source-chart contract. It is intentionally line-oriented so both a person and the future browser editor can create it without a binary tool:

```text
schemaVersion=1
id=example-single
[tempo]
0=142
[notes]
tap=SW,1/3
hold=C,4,12,10
hold=SE,17+1/3,25+1/3,100
```

`SW`, `NW`, `C`, `NE`, and `SE` name the five panels. Beats are exact integers, fractions, or mixed fractions. A hold stores one start, end, and exact requested tick count; its visual length and combo density remain independent. The native loader rejects malformed data, invalid lanes, impossible holds, and unordered tempo segments through the same immutable `Chart` validator used by gameplay. `.stp` remains a supported tap-only import path, while catalog loading selects the parser by file extension.

## 5. Judgement, score, and gauge

### 5.1 Judgement algorithm

On an input press, `JudgementEngine` finds the earliest unjudged candidate in the pressed player's lane.  It calculates:

```text
timingError = inputTime - candidate.hitTime
```

The absolute error is compared to a configurable ruleset.  Initial vertical-slice defaults are deliberately easy to tune:

| Result | Maximum absolute error |
| --- | ---: |
| Perfect | 22 ms |
| Great | 45 ms |
| Good | 90 ms |
| Bad | 135 ms |
| Miss | Later than the miss window |

An input that is too early does not consume the note.  A note becomes a Miss automatically when its miss deadline has passed.  Scroll speed changes only the visual distance, never these time windows.

### 5.2 Score and combo

`ScoreState` is per player and receives immutable judgement events.  It tracks each judgement count, score, current combo, maximum combo, and hold ticks.  The exact numerical scoring table remains a configurable ruleset rather than hard-coded scene logic.

### 5.3 Energy gauge

`LifeGauge` is a gameplay model in the range `0.0` to `1.0`; its renderer is separate.  Judgements apply configurable deltas.  At zero, the stage may fail according to the selected play mode/ruleset.

The visual gauge uses a masked rounded fill, colour zones, smoothing, hit flashes, damage pulses, and a low-life warning.  A shader may be used for the visual mask/gradient, but it must not contain score or life rules.

## 6. Scenes and lifecycle

The old design embeds `Game` and `Result` inside the song menu.  The rebuild uses explicit scenes:

```text
Boot/Loading → MainMenu → SongSelect → Gameplay → Result → SongSelect
                    ↘ Settings / DeviceAssignment
```

- `SongSelect` creates an immutable `PlaySession` with the selected song, chart, speed, mode, and assigned players.
- `GameplayScene` owns audio playback, chart runtime, judgement, active note views, BGA, score, and life gauge.
- On completion, `GameplayScene` creates immutable `ResultData` and is released after its transition.
- `ResultScene` renders only `ResultData`; it does not retain a live game object.
- Song assets can remain in a shared resource cache, so correct scene separation does not require expensive reloading.

## 7. Rendering and BGA

The logical UI canvas is 1280×720.  The output viewport preserves its 16:9 aspect ratio using scaling and letterboxing where necessary.

The State 04 foundation uses a generic Direct2D/DirectWrite scene overlay over the Direct3D 11 background.  A scene supplies semantic `headline`, `detail`, and `instruction` strings; the framework renderer applies the 1280×720 logical transform and never contains scene rules.  This text-only overlay is a development presentation layer, to be replaced by theme-resolved UI resources and animations in later stages.

State 07 adds the first gameplay-field projection: five lanes, receptors, tap heads, and hold bodies are derived from chart seconds and the current song-clock seconds. State 08 then validates the rules with the same monotonic `DebugSongClock`: input edges are compared only as `inputTime - noteTime`, automatic misses occur after the final window, and rendering position is never read back for judgement. The default development windows are Perfect ≤30 ms, Great ≤60 ms, Good ≤100 ms, and Bad ≤150 ms. An FMOD-backed `AudioClock` will replace this adapter in State 09 without changing the judgement engine.

State 09 supplies that replacement through `FmodAudioPlayer` and `FmodSongClock`. A song manifest owns the stable song id, display metadata, original audio filename, and an explicit millisecond offset; the C++ code does not infer titles from files. Music lives below ignored `assets/local/`, while manifests remain versioned. The FMOD SDK path is a local CMake setting, and its runtime DLL is copied next to the executable only for local builds.

State 10 imports the legacy five-lane `.stp` tap format without editing its source file. Each `1` in the first lane's fixed sixteenth-note grid becomes a `TapNote`; the importer deliberately preserves the old loader's two compatibility details: header counts may equal the first-lane length plus one, and trailing symbols beyond the first lane's length are ignored. The old `startPoint` is not an audio-device offset. It was a Y coordinate: with the old 45-pixel grid, 3×BPM note velocity, and receptor at Y=83, it becomes a chart lead-in of `(startPoint - 83) / 180` beats. Score judgements update a basic 0–100 energy state, and FMOD playback completion moves the session into Result with its score, max combo, and clear/fail state.

State 11 adds a static BGA path to the song manifest. `SceneOverlayRenderer` loads it through WIC only while gameplay is active; a missing or unreadable file preserves the dark generated fallback instead of failing the session. The same field now contains a simple vertical 0–100 energy gauge. These are intentionally basic presentation adapters: video BGA, dynamic scene animation, and skinned gauge effects remain separate Phase 3 systems.

State 12 makes long-note authoring executable rather than only structural: `.pdxchart` loads exact tuplet beats, tempo changes, tap events, and holds with independent fixed tick counts. It deliberately does not activate hold scoring yet; State 13 consumes the resolved hold head/tick/end timeline during live play.

State 13 connects those holds to live gameplay. A successful head starts its per-lane hold state; a press during an already-passing body catches the hold from its next available sustain point. Evenly distributed sustain points, including one at the end, emit `HoldTick` or `HoldEnd` judgement events. The score, combo, gauge, and result summary receive those events just like taps, while `ScoreState` also records the total number of sustain points. Releasing a hold misses only the points that pass while the panel is up; pressing again resumes from the next point and never revives earlier points. This PIU-style re-hold rule is clear and testable without tying timing to rendering.

State 14 adds a versioned native hold overlay for the local `NewSongToGod` audio. The current manifest still loads `NewSongToGod_10.stp` unchanged, then merges only the overlay's original hold events into its runtime chart. F5 therefore preserves the legacy tap stream while adding short/long holds, 8/12/16/20-point sustain densities, and two simultaneous holds on different panels. The Direct2D placeholder field now distinguishes a caught hold body from an inactive one using the active palette colour. Final note skins, burst effects, and animation are still theme work rather than chart rules.

State 15 tightens the lane contract after validating the real legacy tap stream: a tap cannot share its lane with a hold at any point from that hold's head through its end. The `NewSongToGod` overlay is placed only inside measured empty spans of its matching legacy lane, while different lanes may overlap freely. This turns a visual/gameplay convention into a deterministic chart-validation error rather than leaving it to manual authoring discipline.

State 16 replaces the development field placement with the fixed 1280×720 single-player gameplay layout. `GameplayLayout` is the shared source of truth for both chart projection and Direct2D rendering: the centred five-lane field spans X=290–990, the receptor line is Y=498, and the field is clipped from Y=156–672. A horizontal energy bar sits directly above the field, song information and score occupy the top band, and judgement/combo feedback is centred above the receptors. The HUD exposes score, combo, max combo, and hold-tick count without putting gameplay state in the renderer. Receptors brighten while pressed; inactive holds are muted, caught holds use the active colour, and a hold that has missed a sustain point receives an amber/red warning outline. These are semantic placeholder visuals; final arrows, skins, particles, and animated transitions remain theme-driven work.

State 16-1 adds PIU-style consumed-hold rendering. While an active hold is currently pressed and its head has crossed the receptor, the renderer hides that passed head and clamps the body start to the receptor line; only the remaining tail is drawn, so the visible hold shortens continuously. If the panel is up, the body is not clamped and proceeds above the receptor, making the missed passage visible. A final sustain point completed while held removes the hold from the field immediately. This view behaviour derives solely from gameplay hold state and audio time; it never changes a judgement result.

State 16-2 repositions the 1P HUD using the supplied Pump It Up gameplay reference: the narrow five-lane field is left anchored at X=162–734, the receptor line is lifted to Y=152, and notes rise from the lower field into that top receptor band. The horizontal energy bar is aligned above it at X=188–680 / Y=20–46. Judgement appears directly below the receptors and combo below the judgement; song information and score move to the upper-right, leaving the surrounding BGA readable. This supersedes the centred, lower-receptor State 16 placement while keeping the same shared `GameplayLayout` timing/render contract.

The gameplay render order is:

```text
BGA video/static fallback → readability overlay → note field → receptors/pad effects
→ judgement/combo effects → per-player HUD → transition overlay
```

BGA video is selected from the same `AudioClock` as the chart.  If decoding falls behind, late video frames are dropped rather than slowing gameplay or audio.  If video is unavailable, the static fallback remains playable.

Single-player places the five-lane field centrally.  Versus/co-op use independent P1 and P2 viewports on the left and right while sharing the BGA.

### 7.1 Scene UI motion

BGA video is not the only motion in the game.  Every scene can own a small `SceneTimeline` made from reusable UI animation primitives: sprite-frame sequences, position/scale/rotation/opacity tweens, easing curves, and timed callbacks.  Animations are described by an atlas/animation definition rather than scattering individual image-loading code through scenes.

There are two intentionally separate clocks:

- **Gameplay/beat clock:** note effects, receptors, combo pulses, and beat-reactive gameplay visuals follow `AudioClock`.
- **UI clock:** menu backgrounds, selection cursors, panel entrances/exits, button hover/press states, and result count-up animations follow normal UI time and remain usable outside a song.

Initial scene direction:

- `MainMenu`: subtle looping background motion, logo entrance, and responsive Play/Settings/Quit button states.
- `SongSelect`: banner carousel slide, selected-song focus/defocus, difficulty-selector movement, and confirm/cancel transitions.
- `Gameplay`: pad lights, receptors, judgement bursts, combo pulses, gauge impact feedback, and beat-reactive HUD accents in front of the BGA.
- `Result`: staged count-up of each result row, grade reveal, and a return prompt after the reveal finishes.
- `DeviceAssignment/Settings`: device cards, input-test flashes, and clear assignment confirmation feedback.

UI motion must be interruptible: a scene transition cancels or completes its timeline safely, and the Reduced Motion setting may simplify nonessential effects.

## 8. Input and players

Each physical controller has its own `InputDevice` and button states.  Device input is not merged globally.

`DeviceAssignmentScene` asks the user to press the centre panel on the physical pad intended for P1, then P2.  It stores a device path/GUID when stable identification is available.  Identical pads without a stable serial must be assigned again after reconnecting; software cannot infer their physical left/right placement.

The runtime supports these modes from its data model:

- single player: one five-panel device;
- versus: P1 and P2 each have one device and independent scoring;
- co-op: two device inputs mapped to a shared play session;
- future double: one player may own two devices and ten lanes.

Input offset, binding tests, remapping, device reconnect handling, and keyboard fallback belong in Settings/Device Assignment rather than gameplay code.

## 9. Runtime performance model

- Whole charts and song assets are loaded before gameplay begins.
- Only notes within the approach/visible time window have an active render view.
- `NoteViewPool` recycles finished tap/hold views.
- `EffectPool` recycles judgement, combo, pad-light, and particle effects.
- The active set alone is updated and rendered each frame.
- Chart data remains immutable; only gameplay state changes during a song.

This preserves the useful part of the legacy preloading approach while avoiding permanent per-note objects and full-chart updates every frame.

## 10. Browser chart editor

The editor is a separate browser application, not a runtime scene.  Its timeline is measure/beat based and supports:

- adjustable grid snap per measure or selection: 4, 8, 12, 16, 24, 32, arbitrary N, and no snap;
- triplets and other tuplets through rational beat positions;
- BPM/time-signature visual markers;
- tap placement, drag-to-create holds, hold resize, and per-hold tick policy;
- audio preview, waveform, beat metronome, scroll/zoom;
- undo/redo, copy/paste, validation, and export/compile;
- import of legacy fixed-grid `.stp` charts.

The 2023 `stepCreator2.html` is retained as a useful prototype reference only.  It has a fixed 58-bar, 16-row-per-bar table and exports five strings of `0`/`1`; it cannot express the new timing or hold model.

## 11. First playable vertical slice

The first implementation milestone is complete only when all of the following work in one test song:

1. 1280×720 logical layout scales correctly in a resizable/fullscreen window.
2. Audio starts after loading and is the authoritative clock.
3. A single player can choose a test chart and play taps at multiple scroll speeds without a judgement-window change.
4. Notes, input, misses, combo, score, and ResultScene use the timing model described above.
5. The song has a static BGA fallback; video integration may then be enabled without changing gameplay timing.
6. Automated tests cover beat-to-time conversion and judgement boundaries.

Long notes, BGA video, two-player assignment, and the editor are later milestones built on this slice.

## 12. Selected runtime technology

The runtime uses a Windows-native C++20 stack.  The initial project is built with CMake and Direct3D 11; later phases add FMOD for music playback, Media Foundation for BGA video, and Raw Input/DirectInput at the Windows controller boundary.  This preserves the original project's native C++ character without reusing its Direct3D 9-era architecture.

Direct3D 11 and CMake are present from the bootstrap.  FMOD, video playback, and physical pad support are introduced only in their relevant phases so the first build has a small, verifiable dependency surface.

## 13. Object and file structure

The rebuild is intentionally multi-file and responsibility-oriented.  A file is not split merely to be small, but no scene, device, or gameplay object may become a catch-all manager.  The initial native-project layout is:

```text
PumpDX_Rebuild/
├─ docs/                         # Design and user-facing technical documents
├─ assets/                       # Versioned development assets; not C++ source
├─ src/
│  ├─ framework/                 # Reusable technical base; no PumpDX rules
│  │  ├─ render/                 # Renderer, viewport, sprites, video surface
│  │  ├─ audio/                  # AudioClock, MusicPlayer, audio asset loading
│  │  ├─ input/                  # Devices, bindings, Raw Input/DirectInput boundary
│  │  ├─ assets/                 # ResourceCache, manifests, theme package loading
│  │  └─ platform/windows/       # Window and Windows-specific adapters
│  └─ game/                      # PumpDX-specific rules and presentation
│     ├─ application/            # PumpDX startup and game-flow composition
│     ├─ chart/                  # Chart model, TimingMap, compiler, validation
│     ├─ content/                # Song metadata and catalogues
│     ├─ gameplay/               # JudgementEngine, ScoreState, LifeGauge, note runtime
│     ├─ scenes/                 # One folder per scene and scene-local presentation
│     │  ├─ main_menu/
│     │  ├─ song_select/
│     │  ├─ gameplay/
│     │  ├─ result/
│     │  └─ device_assignment/
│     ├─ session/                # PlaySession and immutable ResultData
│     └─ ui/                     # Layout, SceneTimeline, and UI animation
├─ tools/                         # Separate browser chart/theme editor projects
└─ tests/                         # Executable-level test data and test harnesses
```

`src/framework` is intentionally a first-class boundary, matching the useful separation in the original project: it owns reusable platform and technical services, while `src/game` owns PumpDX-specific rules, chart semantics, and scene presentation.  The game may depend on framework interfaces; framework code never depends on PumpDX game rules.  The eventual C++ implementation normally gives a stateful public object its own header/source pair (for example, `JudgementEngine.hpp` and `JudgementEngine.cpp`).  Small, inseparable value types may share a file (`JudgementTypes.hpp`), and implementation-only helpers remain private to their `.cpp` file.

Responsibilities are deliberately bounded:

- `GameApplication` bootstraps services and owns the loop; it does not contain game rules.
- `SceneManager` owns only scene transitions and the current scene; it does not own scores or live device state.
- `GameplayScene` composes gameplay services for a session but delegates judgement, scoring, gauge, BGA, HUD, and note views to their own objects.
- `JudgementEngine` decides timing results only; `ScoreState` and `LifeGauge` react to those results separately.
- `TimingMap` converts musical beats and seconds; render code never reimplements BPM mathematics.
- `PadDevice` owns one physical controller; player assignment maps devices to players without merging their inputs.
- `ResourceCache` and `ThemeManifest` resolve assets; scenes never hard-code asset file paths.

Dependencies flow inward: platform/render/audio adapters may be used by gameplay and scenes through narrow interfaces, while core chart and judgement rules must not depend on a window, texture, or scene.  Global singleton-style managers and a universal `GameManager` are avoided.  This keeps unit testing and later replacement of video/input/render backends realistic.

Each feature change must name its owning domain and add files there rather than extending an unrelated class.  New source files are accompanied by the smallest relevant test or test fixture when the domain has observable logic.

## 14. Skin and theme system

Visual assets are replaceable through versioned theme packages.  A theme changes presentation without changing chart timing, judgement rules, input bindings, or the underlying gameplay layout contract.

```text
themes/<theme-id>/
├─ theme.json                     # Schema version, metadata, slot mapping
├─ menu/                          # Backgrounds, logos, buttons, selectors
├─ gameplay/                      # Notes, hold bodies, receptors, pad lights, HUD
├─ judgement/                     # Judge, combo, and effect sprite sheets
├─ result/                        # Result backgrounds, grades, number skins
├─ settings/                      # Device assignment and settings artwork
└─ fallback/                      # Optional BGA/static fallback presentation
```

`theme.json` maps stable semantic keys (for example `gameplay.note.sw`, `ui.song_select.cursor`, or `result.grade.a`) to files and their presentation metadata.  Metadata includes the logical design size, anchor/pivot, sprite-sheet grid or frame rectangles, animation timing/looping, nine-slice data where relevant, and an optional colour/opacity default.  Code refers to semantic keys only; scenes never reference theme file paths directly.

State 05 provides the first executable subset of this contract: schema version, theme id, five overlay-palette colours, and `resources.<semantic-key>` mappings are loaded from the active `theme.json`.  `ResourceCache` resolves only relative resource paths and rejects paths that leave the selected theme package.  Sprite metadata, image decoding, and hot reload remain later additions to the same schema rather than a separate theme system.

The renderer preserves gameplay geometry independently of bitmap dimensions.  Replacing a note image cannot move its judgement location: the note/receptor anchors are part of the layout contract, while a theme supplies its visual pivot/offset.  The theme validator checks required keys, readable files, frame bounds, supported video/image formats, and 1280×720 logical-size compatibility.  Missing optional assets fall back to the active default theme; missing required assets prevent activation and report a clear error.

`ThemeManifest` and `ResourceCache` load the selected theme at scene/session boundaries.  Development builds may hot-reload a changed manifest or asset safely; released builds use a prepared package.  BGA remains song-owned by default, while a theme may provide a static fallback or visual overlay.

The future browser `Theme Editor` works with the same manifest and provides, in stages:

1. slot-based image replacement, preview, scale/crop, anchor, tint, and opacity controls;
2. sprite-sheet slicing and UI/effect animation definitions;
3. per-scene live preview, validation, package import/export/duplication, and a test-theme activation flow.

Theme packages have a schema version and can declare their game-version compatibility.  They contain only assets the creator owns or is licensed to distribute.
