# Real native blueprint fixtures

Exported with Alchemy Factory 1.0.4963 during FountainControl 0.1.2 integration
checks. The two upright fountains have source SettleKeys **28916** and **73598**
in the original world; these are provenance, not keys to assume in a new world.
No player save is needed to inspect/import the files.

- `fountains-with-settings.png`: original export formerly named
  `QWcdKE1cN5ZvoUqeZghvjg.png`; one `fcRs` chunk, building count 2, entries
  index 0 = 700/300 cm and index 1 = 350/150 cm (drop/horizontal).
- `fountains-without-settings.png`: the exact same canonical PNG/native building
  tail with mod metadata removed, formerly `vanilla-preview-test.png`.
- `manifest.json`: file SHA256s, canonical SHA256, native footer and expected
  settings. Canonical bytes of both files are identical; neither is a screenshot
  replacement or a synthetic building record.

Run `python3 tools/inspect_blueprint.py --check-fixtures` from the repo root.
The C++ codec's malformed-input cases are in `tests/blueprint.cpp`. For actual
placement and collision/preview/mod-free checks, see
[docs/testing.md](../../../docs/testing.md). Copy the game-exported files directly;
image editors may strip the chunk or native tail. Both should remain ordinary
native blueprints when the mod is absent.

## 1.1.5024 fixtures

The `1.1.5024/` pair was exported and title/description edited through the normal
UI on 2026-10-16 with mod 0.1.8. Source keys 70/71 belong only to the fresh
disposable port world. The metadata-free variant removes only `fcRs`; canonical
PNG and native tail remain identical. Its own manifest records the hashes.
Both the old and new fixture pairs are checked by `--check-fixtures`.
