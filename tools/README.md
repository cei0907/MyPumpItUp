# Tools

This folder contains independent browser applications for chart authoring and theme editing. They are intentionally separate from the game runtime so authoring tools do not consume gameplay resources.

## Chart Editor

`chart-editor/` is a local browser editor for the readable `.pdxchart` source format. It is not a game runtime scene and is not intended for public hosting.

```powershell
cd tools/chart-editor
npm run dev
```

Open the local address shown by the command. In State 21 it can:

- load an existing `.pdxchart` from the local file system;
- show its five lanes and existing tap/hold events;
- add, select, remove, undo, and redo integer-beat tap notes;
- create long notes by dragging downward in a single lane, resize a selected long-note tail, and edit its tick count;
- edit chart id and initial BPM;
- write the same `.pdxchart` format accepted by `NativeChartLoader`.

After State 23, open the full converted local charts from `assets/local/charts/converted/`. They contain every legacy tap and no added long notes. The tracked `assets/charts/new-song-to-god-hold-playtest.pdxchart` remains a separate, optional long-note authoring sample.

On Edge or another browser that supports the File System Access API, an opened chart saves back to the selected file. Other browsers download the edited file, which can then replace the source chart manually.

The timeline is intentionally vertical: time moves downward through the five fixed columns, and the visible Canvas is the only note rendering surface. This remains responsive even when a chart has many events.
