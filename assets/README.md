# Assets

Version-controlled assets must be small, intentional examples or assets with clear redistribution rights.

Large local media, unlicensed media, generated output, and recordings belong in the ignored paths documented by `.gitignore`.

## Restoring the legacy local collection

The tracked manifests in `assets/catalog/` describe the four songs that were registered in the original SongMenu: Africa, Flying Dock, New Song To God, and You Are Good. Their MP3 files and STP charts remain local-only. To restore them for development, copy only these files from `DxPumpV0.76`:

- `music/Africa.mp3`, `music/FlyingDock.mp3`, `music/NewSongToGod.mp3`, `music/YouAreGood.mp3` to `assets/local/songs/legacy/`
- `step/Africa_16.stp`, `step/FlyingDock_3.stp`, `step/FlyingDock_5.stp`, `step/NewSongToGod_10.stp`, `step/YouAreGood_14.stp` to `assets/local/charts/legacy/`

The catalog scans every `*.song.manifest` file, groups entries with the same `id` as difficulty choices, and never commits those local media files.

## Local BGA video

Keep owner-supplied BGA videos outside Git below `assets/local/bga/`. For example, place a file at `assets/local/bga/legacy/NewSongToGod.mp4`, then add this optional line to its version-controlled song manifest:

```text
videoBgaPath=../local/bga/legacy/NewSongToGod.mp4
```

During gameplay the video follows the FMOD audio clock. If the file is missing, unsupported, or cannot be decoded, the game keeps using `staticBgaPath` instead. Do not add music or BGA media without clear ownership or distribution permission.
