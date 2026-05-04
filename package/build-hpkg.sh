#!/usr/bin/env bash
#
# build-hpkg.sh — cross-build NFSMount and produce a standalone .hpkg
#
# Runs on the Haiku Linux cross-build server (kevin@192.168.74.122).
# Expects the project tree (src/, package/) to be present at the script's
# parent directory.
#
# Output: build/nfsmount-<version>-<arch>.hpkg
#
# Override paths via env vars if needed:
#   HAIKU_BUILD   — top of the haiku source checkout (default: ~/haiku-build/haiku)
#   HAIKU_ARCH    — target arch (default: x86_64)

set -euo pipefail

HAIKU_BUILD="${HAIKU_BUILD:-$HOME/haiku-build/haiku}"
HAIKU_ARCH="${HAIKU_ARCH:-x86_64}"

GENERATED="$HAIKU_BUILD/generated.${HAIKU_ARCH}"
CROSS_BIN="$GENERATED/cross-tools-${HAIKU_ARCH}/bin"
HOST_TOOLS="$GENERATED/objects/linux/${HAIKU_ARCH}/release/tools"

CXX="$CROSS_BIN/${HAIKU_ARCH}-unknown-haiku-g++"
RC="$HOST_TOOLS/rc/rc"
XRES="$HOST_TOOLS/resattr/resattr"
MIMESET="$HOST_TOOLS/mimeset/mimeset"
PACKAGE="$HOST_TOOLS/package/package"

# Haiku-side libs and headers
HAIKU_KITS="$GENERATED/objects/haiku/${HAIKU_ARCH}/release/kits"
HAIKU_HEADERS="$HAIKU_BUILD/headers"

# haiku_devel.hpkg/contents/develop/lib has the CRT files (crti.o, crtn.o,
# start_dyn.o, init_term_dyn.o) and static .a archives. The .so symlinks in
# this dir point at ../../lib/libX.so, which only resolves once the packages
# are installed — at build time we pull the real .so files out of the
# adjacent haiku.hpkg contents tree instead.
HAIKU_DEVEL_LIB="$GENERATED/objects/haiku/${HAIKU_ARCH}/packaging/packages_build/regular/hpkg_-haiku_devel.hpkg/contents/develop/lib"
HAIKU_RUNTIME_LIB="$GENERATED/objects/haiku/${HAIKU_ARCH}/packaging/packages_build/regular/hpkg_-haiku.hpkg/contents/lib"

# gcc_syslibs_devel (libstdc++ etc.) — pick the only matching dir to avoid
# pinning the gcc version string. Same trick as haiku_devel: the .so symlinks
# in develop/lib point at ../../lib/, which only resolves once gcc_syslibs is
# also installed. So we add both the develop/lib (for static archives) and
# the sibling gcc_syslibs/lib (for the actual shared objects) to -L.
GCC_SYSLIBS_DEVEL=$(ls -d "$GENERATED/build_packages"/gcc_syslibs_devel-*-"${HAIKU_ARCH}"/develop/lib 2>/dev/null | head -1)
GCC_SYSLIBS=$(ls -d "$GENERATED/build_packages"/gcc_syslibs-*-"${HAIKU_ARCH}"/lib 2>/dev/null | head -1)
if [ -z "$GCC_SYSLIBS_DEVEL" ] || [ -z "$GCC_SYSLIBS" ]; then
	echo "ERROR: gcc_syslibs[_devel] not found under $GENERATED/build_packages/" >&2
	exit 1
fi

# Project paths
PROJ="$(cd "$(dirname "$0")/.." && pwd)"
SRC="$PROJ/src"
BUILD="$PROJ/build"
PKG_ROOT="$BUILD/package_root"
PKG_INFO_TEMPLATE="$PROJ/package/PackageInfo"

# ------------------------------------------------------------------
# Sanity checks
# ------------------------------------------------------------------
for tool in "$CXX" "$RC" "$PACKAGE"; do
	if [ ! -x "$tool" ]; then
		echo "ERROR: required tool not found or not executable: $tool" >&2
		echo "       Has the Haiku build tree been built? Try:" >&2
		echo "         cd $GENERATED && jam -q -j4 \\<build\\>haiku_devel.hpkg" >&2
		exit 1
	fi
done

if [ ! -f "$HAIKU_KITS/libbe.so" ]; then
	echo "ERROR: libbe.so not found at $HAIKU_KITS/" >&2
	echo "       Run a full Haiku build first: jam -q -j4 @nightly-anyboot" >&2
	exit 1
fi

# ------------------------------------------------------------------
# Clean staging
# ------------------------------------------------------------------
rm -rf "$BUILD"
mkdir -p "$BUILD" "$PKG_ROOT/apps"

# ------------------------------------------------------------------
# Compile
# ------------------------------------------------------------------
SOURCES=(
	App.cpp
	Constants.cpp
	EditShareWindow.cpp
	MainWindow.cpp
	Settings.cpp
	ShareItem.cpp
	ShareManager.cpp
)

INCLUDES=(
	-I"$HAIKU_HEADERS"
	-I"$HAIKU_HEADERS/os"
	-I"$HAIKU_HEADERS/os/app"
	-I"$HAIKU_HEADERS/os/interface"
	-I"$HAIKU_HEADERS/os/locale"
	-I"$HAIKU_HEADERS/os/storage"
	-I"$HAIKU_HEADERS/os/support"
	-I"$HAIKU_HEADERS/os/kernel"
	-I"$HAIKU_HEADERS/posix"
	-I"$HAIKU_HEADERS/config"
	-I"$HAIKU_HEADERS/private/interface"
	-I"$HAIKU_HEADERS/private/shared"
)

CXXFLAGS=(
	-O2 -Wall -Wno-multichar
	-fno-strict-aliasing
	"${INCLUDES[@]}"
)

OBJS=()
echo "==> Compiling..."
for s in "${SOURCES[@]}"; do
	obj="$BUILD/${s%.cpp}.o"
	echo "    $s"
	"$CXX" "${CXXFLAGS[@]}" -c "$SRC/$s" -o "$obj"
	OBJS+=("$obj")
done

# ------------------------------------------------------------------
# Link
# ------------------------------------------------------------------
echo "==> Linking..."
# -B adds HAIKU_DEVEL_LIB to BOTH the library search path AND the startfile
# search path, so the linker finds crti.o / start_dyn.o / etc. alongside libs.
"$CXX" "${OBJS[@]}" \
	-B"$HAIKU_DEVEL_LIB" \
	-L"$HAIKU_DEVEL_LIB" \
	-L"$HAIKU_RUNTIME_LIB" \
	-L"$GCC_SYSLIBS_DEVEL" \
	-L"$GCC_SYSLIBS" \
	-Wl,-rpath-link,"$HAIKU_RUNTIME_LIB" \
	-Wl,-rpath-link,"$GCC_SYSLIBS" \
	-shared-libgcc \
	-lbe -lroot -lstdc++ -lgcc_s \
	"$HAIKU_DEVEL_LIB/libcolumnlistview.a" \
	"$HAIKU_DEVEL_LIB/liblocalestub.a" \
	-o "$BUILD/NFSMount"

# ------------------------------------------------------------------
# Resources
# ------------------------------------------------------------------
echo "==> Compiling resources..."
"$RC" -o "$BUILD/NFSMount.rsrc" "$SRC/NFSMount.rdef"

echo "==> Attaching resources..."
"$XRES" -o "$BUILD/NFSMount" "$BUILD/NFSMount.rsrc"

if [ -x "$MIMESET" ]; then
	echo "==> Setting MIME info..."
	"$MIMESET" -F "$BUILD/NFSMount" || true
fi

# ------------------------------------------------------------------
# Stage the package tree
# ------------------------------------------------------------------
echo "==> Staging package tree..."
cp "$BUILD/NFSMount" "$PKG_ROOT/apps/NFSMount"
chmod +x "$PKG_ROOT/apps/NFSMount"

# Deskbar Applications-menu entry — relative symlink
mkdir -p "$PKG_ROOT/data/deskbar/menu/Applications"
ln -sf ../../../../apps/NFSMount \
	"$PKG_ROOT/data/deskbar/menu/Applications/NFSMount"

# .PackageInfo lives at the package root
cp "$PKG_INFO_TEMPLATE" "$PKG_ROOT/.PackageInfo"

# ------------------------------------------------------------------
# Build the .hpkg
# ------------------------------------------------------------------
VERSION=$(awk '/^version/ { gsub(/[ \t]+/, " "); print $2 }' "$PKG_INFO_TEMPLATE")
HPKG="$BUILD/nfsmount-${VERSION}-${HAIKU_ARCH}.hpkg"

echo "==> Creating $HPKG..."
rm -f "$HPKG"
( cd "$PKG_ROOT" && "$PACKAGE" create -q "$HPKG" )

echo
echo "==> Done"
echo "    $HPKG"
ls -lh "$HPKG"
