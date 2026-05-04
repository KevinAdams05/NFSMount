# build.ps1 — orchestrate cross-build of NFSMount on the Haiku build server
#
# Pushes the source tree to kevin@192.168.74.122, runs build-hpkg.sh,
# and pulls the resulting .hpkg back to \\storage01\files\Temp\Haiku.
#
# Usage:
#   .\package\build.ps1            # build, drop .hpkg on storage01
#   .\package\build.ps1 -KeepRemote # leave the remote tree in place for inspection

[CmdletBinding()]
param(
    [string] $RemoteHost = 'kevin@192.168.74.122',
    [string] $RemoteDir  = '~/build/NFSMount',
    [string] $OutputDir  = '\\storage01\files\Temp\Haiku',
    [switch] $KeepRemote
)

$ErrorActionPreference = 'Stop'

$projectRoot = Resolve-Path (Join-Path $PSScriptRoot '..')
Write-Host "==> Project root: $projectRoot"
Write-Host "==> Remote host:  $RemoteHost"
Write-Host "==> Remote dir:   $RemoteDir"
Write-Host "==> Output dir:   $OutputDir"
Write-Host ""

# --------------------------------------------------------------
# 1. Make sure remote directory exists and is empty
# --------------------------------------------------------------
Write-Host "==> Preparing remote directory..."
& ssh $RemoteHost "rm -rf $RemoteDir && mkdir -p $RemoteDir/src $RemoteDir/package"
if ($LASTEXITCODE -ne 0) { throw "ssh prepare failed" }

# --------------------------------------------------------------
# 2. Copy source + package files up
# --------------------------------------------------------------
Write-Host "==> Uploading source..."
& scp -q -r "$projectRoot\src\*" "${RemoteHost}:$RemoteDir/src/"
if ($LASTEXITCODE -ne 0) { throw "scp src failed" }

Write-Host "==> Uploading package files..."
& scp -q -r "$projectRoot\package\*" "${RemoteHost}:$RemoteDir/package/"
if ($LASTEXITCODE -ne 0) { throw "scp package failed" }

# Ensure the build script is executable
& ssh $RemoteHost "chmod +x $RemoteDir/package/build-hpkg.sh"

# --------------------------------------------------------------
# 3. Run the build remotely
# --------------------------------------------------------------
Write-Host "==> Running cross-build on $RemoteHost..."
& ssh $RemoteHost "bash $RemoteDir/package/build-hpkg.sh"
if ($LASTEXITCODE -ne 0) { throw "remote build failed" }

# --------------------------------------------------------------
# 4. Locate the produced .hpkg and pull it
# --------------------------------------------------------------
Write-Host "==> Locating produced .hpkg..."
$hpkgRemote = (& ssh $RemoteHost "ls $RemoteDir/build/nfsmount-*.hpkg 2>/dev/null | head -1").Trim()
if (-not $hpkgRemote) { throw "no .hpkg produced under $RemoteDir/build/" }
Write-Host "    $hpkgRemote"

if (-not (Test-Path $OutputDir)) {
    throw "Output directory not reachable: $OutputDir"
}

$hpkgName  = Split-Path -Leaf $hpkgRemote
$hpkgLocal = Join-Path $OutputDir $hpkgName

Write-Host "==> Copying to $hpkgLocal..."
& scp -q "${RemoteHost}:$hpkgRemote" "$hpkgLocal"
if ($LASTEXITCODE -ne 0) { throw "scp .hpkg back failed" }

# --------------------------------------------------------------
# 5. Optionally clean up
# --------------------------------------------------------------
if (-not $KeepRemote) {
    Write-Host "==> Cleaning remote build tree..."
    & ssh $RemoteHost "rm -rf $RemoteDir"
} else {
    Write-Host "==> Leaving remote tree at $RemoteDir for inspection"
}

Write-Host ""
Write-Host "==> Done"
Write-Host "    $hpkgLocal"
Get-Item $hpkgLocal | Format-Table Name, Length, LastWriteTime
