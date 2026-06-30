# Changelog

All notable changes to NFSMount will be recorded in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

---

## [0.1.1] – 2026-06-30

Hardens boot-time auto-mount further: the 0.1.0 retry/backoff only helped
when the kernel mount *returned*. A server that was unreachable at boot
could hang `fs_mount_volume()` (which has no timeout), wedging the
auto-mount thread and, in the worst case, leaving the machine off the
network after boot.

### Fixed

- **Pre-flight reachability probe before mounting.** Auto-mount now does a
  short, bounded (3 s) TCP connect to each share's `server:port` before
  calling the kernel mount. An unreachable or not-yet-routable server is
  deferred to a later retry pass instead of being handed to the nfs/nfs4
  add-on, so a down server can no longer wedge the boot-time auto-mount.
- **`_WaitForNetwork()` now ignores link-local addresses.** A link-local
  address (IPv4 169.254/16, IPv6 fe80::/10) is auto-configured before DHCP
  hands out a real lease, so it is no longer treated as "network ready";
  the wait holds out for a routable address. This removes the remaining
  window where mounts could fire into a half-configured network.

---

## [0.1.0] – 2026-06-05

First beta — in daily use for weeks; the alpha label no longer fits.

### Fixed

- **Boot-time auto-mount is now resilient.** Two consecutive
  fresh-boot failures (2026-06-04) showed launch_daemon's
  `network_available` event firing before DHCP had configured an
  address: the nfs4 add-on's RPC connect failed (`Unable to Acquire
  RPCServerManager!` → `Name not found`) and auto-mount gave up after
  its single pass. Two new layers fix this:
  - `_WaitForNetwork()` polls for a configured non-loopback interface
    address (up to 30 s) before the first mount attempt.
  - Auto-mount now runs up to three passes, 5 s apart, retrying shares
    that failed with transient errors (`B_NAME_NOT_FOUND`,
    `B_TIMED_OUT`, `B_IO_ERROR`, connection-refused/unreachable);
    permanent errors (bad configuration) are not retried. Already-
    mounted shares short-circuit, so retry passes are cheap.
  Diagnosis note: this also overturns the May "idmapper" analysis.
  Reading the nfs4 kernel add-on shows it creates the idmapper IPC
  ports **and spawns the daemon itself** as a system team during mount
  (`IdMap::_Repair()`) — a userland-started instance finds no ports
  and exits (verified live: our instance died, the kernel's took
  over). `EnsureIdmapperRunning()` is therefore removed entirely;
  network readiness was the real culprit all along.

### Added

- **Application icon** — NAS box + shared folder with people glyph +
  network tee. Designed as `resources/NFSMount-icon.svg`, converted to
  a 1.4 KB HVIF with hvif-tools' `icon2icon`, embedded via
  `NFSMount.rdef` resource 101 `'VICN'`. Shows in Tracker, Deskbar,
  and automatically in the About window (BAboutWindow renders the
  app's icon resource). Regeneration command documented in the rdef.
- **Mount-point browse button** — the Edit Share window's mount-point
  field now has a "Browse…" button opening a directory-only
  `BFilePanel` (reused across clicks, starts from the current field
  value when it's an existing directory).

### Changed

- Version bumped 0.0.5 → **0.1.0**, variety alpha → **beta**.
- `.gitignore` added (local `Issues/` dumps, build outputs).

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
