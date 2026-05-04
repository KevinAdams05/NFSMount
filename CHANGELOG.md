# Changelog

All notable changes to NFSMount will be recorded in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

---

## [0.0.5] – 2026-05-04

### Added

- **About window** — `Help → About NFSMount…` opens a standard
  `BAboutWindow` showing version, description, copyright, source-code
  URL, the full MIT license text, and a third-party acknowledgements
  section. Also reachable via the Deskbar's "About this app" entry
  (handles `B_ABOUT_REQUESTED` at the application level).
- **`LICENSE` file** at the project root (MIT). The same text is
  embedded in the About window and shipped inside the `.hpkg` under
  `data/documentation/packages/nfsmount/LICENSE`.
- **Help menu** in the main window with the About item.

### Changed

- **MainWindow default size** retuned to **620×330** — large enough
  to show the full column header row, small enough to leave space
  for other windows on a 1024×768 display. Resizable in both axes.
- **EditShareWindow default size** retuned to **450×480** — fits
  the tab content comfortably with the BTabView restructure from
  0.0.4. Resizable in both axes.
- **Width-resize bug fix** — `BCheckBox::MaxSize()` and
  `BStringView::MaxSize()` default to the preferred (text-fit)
  width, which in a vertical layout caps the column's max width
  and locks horizontal resize. Set explicit `B_SIZE_UNLIMITED` max
  width on the four checkboxes in `EditShareWindow` and on the
  `MainWindow` status bar.
- Copyright set to **2026 Kevin Adams** in the `PackageInfo`,
  About window, and `LICENSE`.

---

## [0.0.4] – 2026-04-27 — first public release

### Added

- **Layout rework** — `EditShareWindow` rebuilt around a
  `BTabView` (Basic / Advanced / NFSv2). Window made resizable; the
  height-jump-on-version-switch issue from the BBox-stacking
  approach is gone. State is preserved across version flips.
- **Menu bar** in `MainWindow` with `File`, `Edit`, and `Help` menus.
  Standard Haiku keyboard shortcuts on every action.
- **Import / Export** of share configurations via
  `File → Import shares…` (`Cmd-O`) and `File → Export shares…`
  (`Cmd-S`). On-disk format is a flattened `BMessage` with magic
  `'NFSE'` + version int + array of share messages, file extension
  `.nfsmount`. Duplicate share names are skipped on import rather
  than producing ambiguous duplicates. Export from a newer NFSMount
  is rejected on import to avoid silently dropping unknown fields.
- **Mount / unmount** of NFSv4 shares with one click.
- **Persistent share configurations** stored at
  `~/config/settings/NFSMount` (flattened `BMessage`).
- **Auto-mount** at login via a launch script at
  `~/config/settings/boot/launch/NFSMount`. Failed auto-mounts
  surface in the main window with details rather than failing
  silently.
- **NFSv2 fallback** for servers that don't speak NFSv4.
- **Live status monitoring** detects external mount/unmount changes
  and updates the UI every 10 seconds.
- **Advanced NFS4 tuning** — soft/hard retry mode, timeout,
  retransmissions, port, TCP/UDP, directory cache, metadata cache,
  named-attribute emulation.
- Native Haiku UI built on the Layout API, `BColumnListView`, and
  `BFilePanel`.

### Known limitations at 0.0.4

- No HVIF application icon yet — uses the default generic icon.
- No Deskbar replicant.
- No localization catalog files yet (English-only UI).

---

[0.0.5]: https://github.com/KevinAdams05/HaikuTools/releases/tag/v0.0.5
[0.0.4]: https://github.com/KevinAdams05/HaikuTools/releases/tag/v0.0.4
