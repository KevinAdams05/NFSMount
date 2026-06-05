# NFSMount TODO

## Bugs

- [ ] **Auto-mount still fails intermittently at boot — the 0.0.5-4
  `EnsureIdmapperRunning()` 500 ms snooze is not always enough.**
  Observed 2026-06-04 on the bare-metal test box, two consecutive
  boots, running 0.0.5-4 (which contains the idmapper auto-start fix):
  syslog shows `started nfs4_idmapper_server (team N)` — so the helper
  worked — *immediately* followed by
  `KERN: nfs4: Unable to Acquire RPCServerManager!` and
  `NFSMount: storage01 failed: Name not found`, "0 mounted, 1 failed",
  no retry. The fixed `snooze(500000)` between `load_image()` and the
  first mount loses the race on busy boots (the idmapper hasn't bound
  its IPC ports yet). A manual Mount() later in the session succeeds.
  A second possible contributor on the same boots: DHCP had not
  ACK'd yet when the mount fired (launch_daemon's `network_available`
  may trigger on interface-up rather than address-acquired).
  Fix direction, layered:
  (a) replace the fixed snooze with a readiness poll — retry the
  mount (or probe the idmapper's port) with backoff, e.g. 6 × 500 ms;
  (b) on `B_NAME_NOT_FOUND` specifically, retry the whole auto-mount
  pass once or twice ~5 s apart instead of giving up after one shot;
  (c) longer-term, subscribe to `B_NETWORK_MONITOR` and (re)run
  auto-mount when an address arrives — also covers cable-replug.
  Syslog signature to test against: `started nfs4_idmapper_server`
  followed within ~1 s by `Unable to Acquire RPCServerManager!`.

## Pending

- [x] **Application icon** — done 2026-06-05. Designed as
  `resources/NFSMount-icon.svg` (NAS box + shared folder with people
  glyph + network tee), converted to `resources/NFSMount.hvif` (1.4 KB)
  with hvif-tools' `icon2icon`, wired into `NFSMount.rdef` as
  resource 101 `'VICN'` — verified compiling with `rc` on Haiku.
  Regeneration command documented in the rdef comment.

- [ ] **Localization** — add `.catkeys` catalog files for translating UI
  strings via Haiku's Locale Kit.

- [ ] **Drag and drop** — drag a share from the list to Tracker to open its
  mount point in a file-manager window.

- [ ] **Deskbar replicant** — a small shelf icon in the Deskbar tray showing
  mount status, with a popup menu for quick mount/unmount.

- [ ] **Server discovery** — mDNS/Bonjour browsing for NFS servers on the
  local network.

- [ ] **Mount point browsing** — a file panel for selecting the local mount
  point instead of typing a path.