# NFSMount Coding Style Guide

Our style is based on the the **Haiku Project Coding Guidelines**: <https://www.haiku-os.org/development/coding-guidelines>

When this document and the Haiku guidelines disagree, **this document wins**
for NFSMount code. When this document is silent, defer to Haiku.

---

## 1. Project-specific deviations from Haiku

These are the **only** intentional differences from the upstream Haiku style.
Everything else in this document is a restatement of the Haiku rules for
convenience.

### 1.1 Line length — 140-character soft cap

- **Target:** ≤ 100 columns. Stay here whenever it doesn't hurt readability.
  This matches upstream Haiku and keeps side-by-side diffs comfortable.
- **Soft warning:** the linter warns at **140 columns**.
- **Hard cap:** none, but lines past 140 require a justification in code
  review (typically: an unbroken string literal, a URL, or a generated table
  that wrapping would actually harm).

---

## 2. Indentation and whitespace

- **Tabs** for indenting blocks. Editor tab width is **4** for purposes of
  computing line length and alignment.
- Wrapped lines get **at least one extra tab**, plus one more tab per
  expression nesting level.
- Namespace contents are **not indented** — they sit flush at column 0.
- **Spaces** on both sides of binary operators (`a + b`, `x == y`).
- **No space** between a C-style cast operator and its operand: `(int)x`.
- **Always a space** after a comma.
- Every file ends with a newline.
- No trailing whitespace on any line.

## 3. Naming

| Kind | Convention | Example |
|---|---|---|
| Classes, structs, types, namespaces, functions | `UpperCamelCase` | `PageRenderer`, `LoadDocument` |
| Local variables | `lowerCamelCase` | `pageCount`, `targetView` |
| Member variables | `f` prefix + `UpperCamelCase` | `fPageCount`, `fDocument` |
| Constants | `k` prefix + `UpperCamelCase` | `kMaxZoom`, `kDefaultDpi` |
| Globals | `g` prefix | `gApp`, `gSettings` |
| Statics (file/function scope) | `s` prefix | `sZoomTable` |
| Private methods | `_` prefix | `_RenderPage`, `_BuildOutline` |

Rules:

- No underscores in type or function names (other than the `_` prefix on
  private methods).
- **Descriptive names always beat short ones.** No abbreviations, no
  letter-soup names, even for "obvious" things. Spell it out — the few
  extra characters pay for themselves the first time someone unfamiliar
  reads the code.
  - Variables: `message` not `msg`, `menuItem` not `mi`, `rectangle` not
    `rect`/`r`, `index` not `idx`, `pointer` not `ptr`, `bookmark` not `bk`.
  - File and class names: `AnnotationWindow` not `AWindow` or `AnnotWin`;
    `BookmarkList` not `BKmrkLst` or `BMList`; `PageRenderer` not `PgRndr`.
  - Method names: `LoadAnnotations()` not `LoadAnnots()`;
    `GetCurrentPageNumber()` not `GetCurPgNum()`.
- Exception: a few well-known abbreviations are fine when their full form
  would be noise — `id`, `url`, `NFS`, `dpi`, `rgb`, `min`/`max`, `i`/`j`/`k`
  for tight loop indices. When in doubt, spell it out.
- No articles in names — avoid `aMessage`, `theView`, `MyDraw`. Prefer
  `message`, `view`, `Draw`.
- Avoid ambiguous pairs like `ProcessMessage` / `DoProcessMessage`. Pick one
  verb that says what it actually does.
- All identifiers, comments, and strings in **US English** ("color", not
  "colour"; "license", not "licence").

## 4. Braces and blocks

- **Class / struct** opening brace: same line as the declaration.
- **Function** opening brace: on its own line.
- **`if` / `else` / `for` / `while` / `switch`** opening brace: same line as
  the keyword and condition.
- `else` and `else if` go on a new line, after the closing brace of the
  previous block.
- **Single-statement** `if`/`else`/`for`/`while`: omit the braces, put the
  statement on a new indented line.
- **Multi-statement** blocks: always braces.
- Empty inline functions defined inside a class definition may sit on a
  single line. Empty functions defined outside the class follow the standard
  function format (return type on its own line, brace on its own line).
- After an early `return` (or `break`/`continue`) inside an `if`, do **not**
  write an `else` — the `else` is dead syntax.

```cpp
status_t
NFSDocument::LoadFile(const entry_ref& ref)
{
    if (fLoaded)
        return B_BUSY;

    BFile file(&ref, B_READ_ONLY);
    if (file.InitCheck() != B_OK)
        return file.InitCheck();

    // multi-statement → braces
    if (fOwnerPassword != NULL) {
        fOwnerPassword->Truncate(0);
        delete fOwnerPassword;
        fOwnerPassword = NULL;
    }

    return _Parse(file);
}
```

## 5. Functions

- Return type on its own line, **above** the function name.
- Opening brace on its own line, flush left.
- **Two blank lines** between function definitions.
- Long argument lists: wrap and indent the continuation by **one tab**.

```cpp
virtual const char*
LookupAnnotationLabel(const char* name, const char* path,
    const char* user);
```

## 6. Constructor initializer lists

- Colon on its **own line**, indented one tab.
- Each initializer on its own line, indented one tab.
- Prefer initializer lists over assigning in the body — only put work in the
  body that genuinely cannot be expressed as initialization.

```cpp
NFSView::NFSView(entry_ref* ref, FileAttributes* attrs,
    const char* name, uint32 flags)
    :
    BView(name, flags),
    fAttributes(attrs),
    fDocument(NULL),
    fZoom(kDefaultZoom)
{
}
```

## 7. Blank lines

- **Two blank lines** between functions.
- **Two blank lines** between the include block and any subsequent define
  block, and between defines and the first variable/function.
- **One blank line** between cases in a `switch`.
- **One blank line** after the opening `#define` of a header guard.
- **Two blank lines** before the closing `#endif` of a header guard.
- No blank line between the license/copyright block and the header guard.

## 8. Control flow specifics

### 8.1 If / else

- Always use explicit boolean tests, never rely on implicit truthiness.
  - Pointers: `if (pointer != NULL)`, not `if (pointer)`.
  - Integers: `if (count != 0)`, not `if (count)`.
- Bitmasks always go in parentheses with an explicit comparison:
  `if ((flags & kMask) != 0)`.
- No assignment inside an `if` (or `while`) condition. Split it:
  ```cpp
  status_t status = entry.GetRef(&ref);
  if (status != B_OK)
      return status;
  ```
- Variable goes on the **left** of comparisons: `if (status == B_OK)`,
  never `if (B_OK == status)`. NFSMount does not use Yoda conditions.
- Do not wrap an entire `if` condition in redundant outer parentheses, and
  do not parenthesise each clause:
  `if (a == 3 && b != 4)`, not `if ((a == 3) && (b != 4))`.

### 8.2 Long conditions

When wrapping a long boolean expression, put the **logical operator at the
start** of the next line, not at the end of the previous one:

```cpp
if (document != NULL
    && document->IsLoaded()
    && !document->IsEncrypted()) {
    // ...
}
```

### 8.3 Switch

- `case` labels are indented one tab inside the `switch`.
- The body of each case is indented one further tab.
- One blank line between cases.
- Wrap a case body in `{ }` whenever it declares its own variables.
- Always have a `default:` (even if it just `break;`s).

```cpp
switch (action->getKind()) {
    case actionGoTo:
    {
        LinkDest* dest = ((LinkGoTo*)action)->getDest();
        // ...
        break;
    }

    case actionURI:
        out << "URI: " << ((LinkURI*)action)->getURI()->getCString();
        break;

    default:
        break;
}
```

### 8.4 Loops

- Prefer `for` over `while`-with-assignment. If you find yourself writing
  `while ((x = next()) != NULL)`, refactor to a `for` or pull the assignment
  out.
- Range-based `for` is allowed for STL/poppler containers; use it when the
  index is not needed.

### 8.5 No `goto`. No exceptions for cleanup either — use RAII.

## 9. Types

### 9.1 Prefer Haiku types over raw C types

When working in Haiku-native code (the entire current codebase):

- `int32` / `uint32` instead of `int` / `unsigned`.
- `int64` / `uint64` for explicit 64-bit.
- `off_t` for file offsets.
- `size_t` / `ssize_t` for sizes.
- `status_t` for error returns. **All NFSMount functions that can fail
  return `status_t`**, with `B_OK` on success.

These come from `<SupportDefs.h>`.

### 9.2 Haiku style focus

We are **not** writing portable code today. Haiku-idiomatic code first.
Do not pre-emptively introduce `#ifdef __HAIKU__` blocks anywhere in the
main source tree.


### 9.3 Strings

- `BString` over `char*`, `malloc`/`strdup`/`free`, or fixed `char[N]`
  buffers.
- Use `BString::operator<<` and `BString::SetToFormat` instead of `sprintf`.
- Exception: the wire-protocol layer — see §9.6.

### 9.4 Collections

- `BObjectList<T>` over `BList`. The type-safety and ownership semantics
  catch real bugs.
- `std::vector<T>` is acceptable only inside the poppler-interop layer.
- Exception: the wire-protocol layer — see §9.6.

### 9.5 Casts

- Use C++ casts: `static_cast`, `dynamic_cast`, `const_cast`,
  `reinterpret_cast`.
- C-style casts are only acceptable for primitive numeric conversions and
  must have **no whitespace** after the cast operator: `(int)x`, not
  `(int) x`.
- Down-casts must use `dynamic_cast` when the actual runtime type is not
  statically guaranteed.

### 9.6 Exception — the wire-protocol layer

`src/ExportProtocol.{h,cpp}` is exempt from §9.3 (prefer `BString`) and §9.4
(prefer `BObjectList`). Inside that file only:

- Fixed `char[N]` buffers are allowed for decoding protocol strings, sized
  from the limits the RFC defines (`MNTPATHLEN`, `MNTNAMLEN`).
- Results are handed back through a plain callback rather than a
  `BObjectList`.
- Nothing beyond `<SupportDefs.h>` may be included from the Haiku API — no
  sockets, no Interface Kit, no `BString`.

**Why.** This layer only turns buffers into values and back; it performs no
I/O. Keeping it free of Haiku dependencies means the wire format can be
compiled and driven against a real NFS server from an ordinary POSIX harness,
*before* anything is built for Haiku. That is not a hypothetical benefit: the
MOUNT protocol client was validated byte-for-byte against a live server on
another operating system, so by the time it first compiled on Haiku the only
untested part left was the transport. A bug in XDR parsing is otherwise
extremely tedious to chase through a GUI.

The bounds-checking that justifies the raw buffers is not optional: every read
is validated against the end of the reply, lengths are rejected above the
RFC's limits, and a truncated or hostile reply must produce `B_BAD_DATA`
rather than read out of bounds.

**Scope.** The exception covers the protocol file and nothing else. Its caller
(`src/ExportBrowser.cpp`) adapts the results into `BString` and
`BObjectList` immediately and follows the normal rules. A new file may claim
this exception only if it is likewise pure encode/decode with no I/O, and is
actually reachable from an off-target test harness — otherwise it is just an
excuse for C strings.

## 10. Pointers and null

- `NULL`, not `0` or `nullptr`. (Haiku tradition.)
- Initialize pointers with traditional assignment, not constructor syntax:
  `BView* view = NULL;`, not `BView* view(NULL);`.
- **Pointer asterisk binds to the type**: `BString* fTitle;`, not
  `BString *fTitle;`. This is consistent with how Haiku writes function
  signatures and matches `clang-format`'s `PointerAlignment: Left`.
- Do **not** check for `NULL` before `delete` or `free` — both accept
  `NULL` and the check is noise:
  ```cpp
  delete fIcon;   // not: if (fIcon != NULL) delete fIcon;
  ```

## 11. Boolean conventions

- Use `true` / `false` from C++, never `TRUE` / `FALSE` macros.
- Functions that return success/failure return `status_t` (`B_OK` on
  success), not `bool` — `bool` should mean a genuine yes/no flag, not
  "did it work".

## 12. Returns and parentheses

- Do not parenthesise the return expression: `return result;`, not
  `return (result);`.
- Prefer early returns. Keep happy-path code at one indent level.

## 13. Comments

- Prefer `//` over `/* */`.
- Explain **why**, not what. `i++; // increment i` is noise.
- For genuinely tricky code, describe the constraint or pitfall, not your
  feelings: not `// this is a hack!` but `// poppler returns NULL for empty
  pages even though the spec requires a valid PageRef; we treat it as an
  intentional blank.`
- No author initials in comments. Git already knows.
- No `// TODO: kevin` style markers. Plain `// TODO:` is fine.
- No `#if 0`'d dead code. Delete it; git has the history.
- **Doxygen** (`/*! ... */`) for documenting public/header API surface.
  Used for code comprehension, not end-user documentation — that lives in
  `docs/`.

## 14. Includes

### 14.1 Ordering

Within a source file (`.cpp`), in this order, with **one blank line**
between groups:

1. The corresponding header (`#include "Foo.h"` from `Foo.cpp`).
2. POSIX / standard C headers (`<stdio.h>`, `<stdlib.h>`, ...).
3. C++ standard headers (`<vector>`, `<memory>`, ...) — only when needed.
4. Poppler headers (`<poppler/...>`).
5. Haiku API headers (`<Application.h>`, `<Path.h>`, ...).
6. Haiku private headers (`<private/...>`) — only when unavoidable.
7. Local project headers (`"NFSView.h"`, `"PageRenderer.h"`).

Within each group, **alphabetize** include lines.

### 14.2 Style

- `<angle>` for system / framework / poppler headers.
- `"quoted"` for local project headers.
- Use **C-style header names**: `<string.h>`, `<stdlib.h>` — not
  `<cstring>`, `<cstdlib>`. (Haiku tradition.)
- Avoid path components when the build system makes them unnecessary:
  `<Application.h>`, not `<be/app/Application.h>` (BeNFS code does this;
  do **not** copy that pattern).

## 15. Header files

### 15.1 Layout

```cpp
/*
 * NFSMount: A tool to manage NFS network shares.
 *   Copyright (C) 2026 Kevin Adams <kevinadams05@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *  [...standard GPL v2+ boilerplate, see §16.1 for the full block...]
 */
#ifndef NFS_VIEW_H
#define NFS_VIEW_H


#include <View.h>


class NFSView : public BView {
public:
                            NFSView(entry_ref* ref, const char* name,
                                uint32 flags);
    virtual                 ~NFSView();

private:
            void            _RenderPage(int32 pageNumber);

            BString*        fTitle;
            int32           fCurrentPage;
};


#endif  // NFS_VIEW_H
```

### 15.2 Header-guard rules

- Form: `#ifndef CLASS_NAME_H` / `#define CLASS_NAME_H` / `#endif  // CLASS_NAME_H`.
- The guard immediately follows the copyright block — **no blank line
  between them**.
- **One blank line** after the `#define`.
- **Two blank lines** before the closing `#endif`.
- The closing `#endif` carries a `// CLASS_NAME_H` comment.

### 15.3 Member declaration alignment

- Members and methods inside a class are typically aligned in columns (see
  example above): access-specifier-relative indent, return type column,
  name column. This matches Haiku public-header style.
- For private implementation classes that won't be reviewed against Haiku
  conventions, plain left-aligned declarations are fine.

## 16. Copyright headers

NFSMount is **MIT License**.

### 16.1 New NFSMount source files

```cpp
/*
 * NFSMount: A native Haiku application for managing NFS shares.
 *
 * Copyright 2026, Kevin Adams <kevinadams05@gmail.com>
 * All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */
```


### 16.2 Years

Update the year range when you make a substantive change. `2026` for a
brand-new file; `2026-2027` if you meaningfully edit it next year. Trivial
typo fixes do not bump the year.

## 17. Resource management

- Stack objects over heap objects whenever possible.
- For locks, use the Haiku `AutoLock` template — never `Lock()`/`Unlock()`
  pairs by hand, and **not** `BAutolock` (deprecated in favour of
  `AutoLock`).
- For poppler / non-Haiku C++ objects, use `std::unique_ptr` with a custom
  deleter when poppler doesn't already RAII-manage the object.
- No `goto cleanup:` patterns. RAII or early return.

## 18. Dead code, debug code, and printfs

- No `#if 0` blocks. Delete the code; git keeps history.
- No leftover `printf` / `fprintf(stderr, ...)` for debugging — promote to
  `Trace()` (the project's syslog wrapper) or remove.
- Long-lived diagnostic code lives behind `#if DEBUG` and **must compile
  warning-clean** in both debug and release builds.
- Prefer `ASSERT(condition)` for invariants over ad-hoc `if (!x) abort();`.

## 19. Logging

NFSMount has a project-wide logging helper:

```cpp
Trace(LOG_INFO, "loaded %s, %d pages", path, pageCount);
Trace(LOG_ERR,  "failed to render page %d: %s", page, strerror(errno));
```

Defined in `NFSMount/util/Trace.h`. It writes to the Haiku syslog
(`syslog(3)`). Use this — do not introduce per-file `fprintf(stderr, ...)`
loggers.

Levels follow `<syslog.h>`: `LOG_DEBUG`, `LOG_INFO`, `LOG_NOTICE`,
`LOG_WARNING`, `LOG_ERR`, `LOG_CRIT`.

## 20. Tooling

TO-DO we need a linter/style checker.

## 21. PR checklist

Before opening a PR, verify:

- [ ] `haiku-format` is clean (or the deviation is justified in the PR
      description).
- [ ] `checkstyle-NFSMount.py` is clean.
- [ ] No lines over 140 columns without justification.
- [ ] Public/header API has Doxygen comments.
- [ ] No `printf`/`fprintf` debug leftovers; logging goes through
      `Trace()`.
- [ ] No `#if 0` blocks.
- [ ] Copyright headers present and correct.
- [ ] File ends with a newline.

---

## Appendix A — Quick reference card

```
Indent: TAB (width 4)
Line:   target ≤100, soft warn at 140
Brace:  class same line; function own line; if/for/while same line
Naming: UpperCamel types/funcs, lowerCamel vars, f/k/g/s prefixes, _ private
Pointer: BString* x = NULL;
Cast:   static_cast<T>(x);   (T)x for primitives only
Null:   NULL, no nullptr
Bool:   true/false, never TRUE/FALSE
Bitmask: if ((x & MASK) != 0)
Switch: case indented; { } if vars; default: required
Strings: BString, not char[]/strdup
Errors: status_t, B_OK on success
Logging: Trace(LOG_LEVEL, fmt, ...);
```