# FountainControl agent handoff

Read [docs/maintaining.md](docs/maintaining.md) first. It records the current
state, user decisions, source map and development workflow. For a game update,
continue with [docs/porting.md](docs/porting.md). For live work, use
[docs/testing.md](docs/testing.md); [docs/artifacts.md](docs/artifacts.md)
explains the included evidence and how to regenerate large/local artifacts.
The parent AlchemyModding project and the original conversation are not needed.

- This directory is its own Git repository. Preserve unrelated/user changes;
  commit at meaningful development milestones. Do not push, tag or publish
  unless requested. Do not spawn agents unless requested.
- Native game hooks are gated by a full executable SHA256. Never bypass that
  gate or merely replace the hash to make a new game version start. Verify
  addresses, ABIs, layouts, assets and behavior first.
- Never change the game's save format or property SaveGame flags. Settings live
  in JSON `.sav` companions under `<world>/FountainControl/`; portable blueprint
  settings live inside the PNG. No old companion-path compatibility is wanted.
- Keep all originally reachable receivers within the configured limits. The
  existing flat-topped outer envelope is intentional. Only settled upright
  MovePattern 2 fountains are in scope; multiplayer is not.
- Game-thread polling is required. Do not replace `LoopInGameThreadWithDelay`
  with the earlier failing async scheduler or call `GetCurrentStateName` at menus.
- Live tests must use explicitly disposable `FountainControl_*Dev` worlds.
  Discover current save paths and IDs; never reuse historical user-world paths,
  object pointers or SettleKeys as if they identified today's objects.
- Development commands are opt-in. Restore the installed configuration and
  remove pending commands after testing. Do not replace a DLL in a running game.
- Keep README end-user oriented, technical material in docs. Preserve the MIT
  license, third-party notices and accurate AI disclosure. Record coverage
  limits honestly (Workshop is inspected, not round-trip tested).
