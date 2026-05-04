# NFSMount — Technical Documentation

## Architecture

NFSMount is a native Haiku C++ application built on the BeOS/Haiku API. It
uses the kernel's `fs_mount_volume()` API to mount NFSv4 shares directly,
without shelling out to command-line tools.

### Component Diagram

![Component Diagram](diagrams/doc-component-diagram.svg)

### Class Reference

#### NFSMountApp (App.h / App.cpp)

Entry point. Subclasses `BApplication`.

| Method | Purpose |
|--------|---------|
| `ReadyToRun()` | Opens MainWindow (normal mode) or calls `_AutoMount()` |
| `ArgvReceived()` | Parses `--auto` flag for auto-mount mode |
| `_AutoMount()` | Mounts all auto-mount shares; shows errors or quits |

**Auto-mount mode**: When launched with `--auto`, the app mounts all shares
that have the auto-mount flag set, then exits. If any mount fails, it shows
an alert offering to open the main window. This mode is triggered by the
login launch script.

#### Settings (Settings.h / Settings.cpp)

Manages persistent share configurations.

| Method | Purpose |
|--------|---------|
| `Load()` / `Save()` | Read/write BMessage from/to disk |
| `CountShares()` | Number of saved shares |
| `GetShare(index)` | Retrieve a share's BMessage by index |
| `AddShare()` / `UpdateShare()` / `RemoveShare()` | CRUD operations |
| `WindowFrame()` / `SetWindowFrame()` | Persist window geometry |
| `HasAutoMountShares()` | Whether any share has auto-mount enabled |

**File format**: A single flattened `BMessage` with what code `'NFSM'`. The
message contains:
- `"window_frame"` (BRect) — main window position and size
- `"shares"` (BMessage, repeated) — one nested message per share

Each share message contains string, bool, and int32 fields for all
configuration options. See `Constants.h` for field name constants.

#### ShareManager (ShareManager.h / ShareManager.cpp)

Static utility class for NFS operations. No instances are created.

| Method | Purpose |
|--------|---------|
| `Mount(share)` | Build params, create mount point, call `fs_mount_volume()` |
| `Unmount(path)` | Call `fs_unmount_volume()` |
| `IsMounted(path)` | Compare device IDs of path vs. parent to detect mount |
| `BuildParameterString(share)` | Convert BMessage fields to NFS4 param string |
| `CreateMountPoint(path)` | `create_directory()` with parents |
| `InstallAutoMount()` | Write launch script to boot/launch/ |
| `RemoveAutoMount()` | Delete the launch script |

**Mount detection**: `IsMounted()` works by comparing the `st_dev` field
from `stat()` on the mount point and its parent directory. If they differ,
a separate filesystem is mounted at that path.

**Parameter string format**: The NFS4 kernel add-on
(`src/add-ons/kernel/file_systems/nfs4/kernel_interface.cpp`) parses mount
parameters as:
```
<ip_address>:<export_path> [option] [option] ...
```

Available options: `hard`, `soft`, `timeo=N`, `retrans=N`, `ac`, `noac`,
`xattr-emu`, `noxattr-emu`, `port=N`, `proto=X`, `dirtime=N`.

`BuildParameterString()` only emits non-default options to keep the string
minimal.

#### MainWindow (MainWindow.h / MainWindow.cpp)

Primary UI. Subclasses `BWindow` with Titled look and Normal feel.

**Layout**: Uses `BLayoutBuilder::Group<>` for vertical arrangement:
1. `BColumnListView` (weighted 10x for expansion)
2. Horizontal button group with Mount/Unmount on the left, Add/Edit/Remove
   on the right
3. `BStringView` status bar at the bottom

**Mount status monitoring**: A `BMessageRunner` fires `kMsgCheckStatus`
every 10 seconds. The handler iterates all `ShareItem` rows and compares
cached status with live `IsMounted()` results. Changed rows are updated
and invalidated for redraw.

**Window lifecycle**: On quit, saves window frame to settings and updates
the auto-mount launch script (installs if any share has auto-mount,
removes if none do).

**Message handling**:

| Message | Handler |
|---------|---------|
| `kMsgShareSelected` | Update button enable/disable states |
| `kMsgShareInvoked` | Open edit window (double-click) |
| `kMsgMountShare` | Mount the selected share |
| `kMsgUnmountShare` | Unmount the selected share |
| `kMsgAddShare` | Open blank EditShareWindow |
| `kMsgEditShare` | Open EditShareWindow with share data |
| `kMsgRemoveShare` | Confirm and remove share from settings |
| `kMsgShareSaved` | Handle save from EditShareWindow |
| `kMsgCheckStatus` | Periodic mount status refresh |

#### EditShareWindow (EditShareWindow.h / EditShareWindow.cpp)

Modal dialog for adding or editing a share. Title changes based on context
("Add NFS Share" vs "Edit NFS Share").

**Layout**: Vertical group containing:
1. Grid of labeled text controls (name, server, export, mount point)
2. Checkboxes (read-only, auto-mount)
3. BBox "Advanced Options" containing a sub-grid of tuning parameters
4. Cancel/Save button row

**Validation**: `_ValidateFields()` checks:
- Name is non-empty
- Server is non-empty
- Export path starts with `/`
- Mount point starts with `/`

Validation failure shows a `BAlert` and focuses the offending field.

**Communication**: On save, posts a `kMsgShareSaved` message to the target
window (MainWindow) containing the share BMessage and the edit index (-1
for new shares).

#### ShareItem (ShareItem.h / ShareItem.cpp)

`BRow` subclass for `BColumnListView`. Each instance represents one saved
NFS share.

**Columns**:

| Index | Column | Content |
|-------|--------|---------|
| 0 | Name | Friendly share name |
| 1 | Server | NFS server address |
| 2 | Export | Export path on server |
| 3 | Status | "Mounted" or "Unmounted" |

**State**: Stores the share's settings index (`fIndex`) for mapping back to
`Settings`. Caches mount status (`fMounted`) to avoid redundant stat calls.

#### Constants (Constants.h)

Defines all shared constants:
- Application signature and name
- NFS4 default values (timeout, retrans, port, protocol, dirtime)
- BMessage `what` codes for inter-window messaging
- Settings field name strings
- Status check interval

---

## Kernel API Reference

### fs_mount_volume()

```c
#include <fs_volume.h>

dev_t fs_mount_volume(
    const char* where,        // Local mount point path
    const char* device,       // NULL for network filesystems
    const char* filesystem,   // "nfs4"
    uint32 flags,             // 0 or B_MOUNT_READ_ONLY
    const char* parameters    // NFS4 parameter string
);
```

Returns a positive device ID on success, or a negative `status_t` error code.

### fs_unmount_volume()

```c
status_t fs_unmount_volume(
    const char* path,         // Mount point to unmount
    uint32 flags              // 0 or B_FORCE_UNMOUNT
);
```

Returns `B_OK` on success.

### Key Header Files

| Header | Contents |
|--------|----------|
| `<fs_volume.h>` | `fs_mount_volume()`, `fs_unmount_volume()` |
| `<FindDirectory.h>` | `find_directory()`, `B_USER_SETTINGS_DIRECTORY` |
| `<Directory.h>` | `create_directory()` |
| `<ColumnListView.h>` | `BColumnListView`, `BRow` (private/interface) |
| `<ColumnTypes.h>` | `BStringColumn`, `BStringField` (private/interface) |

---

## Settings File Format

**Path**: `~/config/settings/NFSMount`

**Format**: Flattened `BMessage` (binary). Can be inspected with Haiku's
`listattr` or by writing a small tool that unflattens and prints it.

**Schema**:

![Settings Schema](diagrams/doc-settings-schema.svg)

---

## Auto-Mount Mechanism

When any share has the "Mount automatically at login" flag set, NFSMount
installs a shell script at:

```
~/config/settings/boot/launch/NFSMount
```

This directory is scanned by Haiku at login. The script contains:

```bash
#!/bin/sh
"/path/to/NFSMount" --auto
```

The path is determined at runtime via `be_app->GetAppInfo()`.

**Startup flow with --auto**:

1. `ArgvReceived()` sets `fAutoMode = true`
2. `ReadyToRun()` calls `_AutoMount()` instead of opening the window
3. `_AutoMount()` iterates all shares, mounting those with `autoMount = true`
4. If all succeed: quit silently
5. If any fail: show an alert with the error list, offering to open the
   main window or dismiss

The launch script is automatically removed when no shares have auto-mount
enabled (checked on every settings save and window close).

---

## Build System

NFSMount uses Haiku's `makefile-engine`, the standard build system for
third-party Haiku applications.

**Dependencies**:
- `libbe.so` — Core Haiku API (BApplication, BWindow, BMessage, etc.)
- `libcolumnlistview.so` — BColumnListView widget
- `libtracker.so` — Tracker shared headers
- `liblocalestub.a` — Locale Kit stub (for future localization)

**Private headers**: `BColumnListView` and `BColumnTypes` are in Haiku's
private interface headers at
`/boot/system/develop/headers/private/interface/`.

**Build commands**:

```bash
# Debug build
make

# Release build (optimized)
make OPT_LEVEL=FULL

# Clean
make clean
```

---

## Future Work (Phase 3)

Items deferred from the current implementation:

- **Application icon**: Design an HVIF icon in Icon-O-Matic depicting a
  network folder. Add to NFSMount.rdef as a VICN resource.

- **About window**: Standard Haiku About dialog showing version, author,
  and license.

- **Localization**: Add `.catkeys` catalog files for translating UI strings
  via Haiku's Locale Kit.

- **Drag and drop**: Drag a share from the list to Tracker to open its
  mount point in a file manager window.

- **Deskbar replicant**: A small shelf icon in the Deskbar tray showing
  mount status, with a popup menu for quick mount/unmount.

- **NFSv3 support**: Add a protocol version selector that switches between
  `nfs4` and `nfs` filesystem types with appropriate parameter formats.

- **Server discovery**: mDNS/Bonjour browsing for NFS servers on the local
  network.

- **Mount point browsing**: A file panel for selecting the local mount point
  instead of typing a path.

---

## References

- [Haiku API Documentation](https://www.haiku-os.org/docs/api/)
- [Haiku Human Interface Guidelines](https://www.haiku-os.org/docs/HIG/index.xml)
- [Haiku Coding Guidelines](https://www.haiku-os.org/development/coding-guidelines)
- [NFSv4 Client Blog Post](https://www.haiku-os.org/blog/pawe%C5%82_dziepak/2013-03-15_nfsv4_client_finally_merged/)
- [Haiku makefile-engine](https://www.haiku-os.org/documents/dev/makefile-engine)
- NFS4 kernel add-on source: `src/add-ons/kernel/file_systems/nfs4/`
- fs_mount_volume() header: `headers/os/kernel/fs_volume.h`
