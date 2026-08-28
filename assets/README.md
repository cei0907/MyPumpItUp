# Assets

Version-controlled assets must be small, intentional examples or assets with clear redistribution rights.

Large local media, unlicensed media, generated output, and recordings belong in the ignored paths documented by `.gitignore`. Song packages and theme packages will be defined after the first playable vertical slice is working.

## Local BGA video

Keep owner-supplied BGA videos outside Git below `assets/local/bga/`. For example, place a file at `assets/local/bga/legacy/NewSongToGod.mp4`, then add this optional line to its version-controlled song manifest:

```text
videoBgaPath=../local/bga/legacy/NewSongToGod.mp4
```

During gameplay the video follows the FMOD audio clock. If the file is missing, unsupported, or cannot be decoded, the game keeps using `staticBgaPath` instead. Do not add music or BGA media without clear ownership or distribution permission.
