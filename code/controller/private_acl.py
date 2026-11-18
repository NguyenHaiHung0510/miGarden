"""Private offline artifacts: establish/verify access before secrets are written."""
from __future__ import annotations
import json
import os
from pathlib import Path
import stat
import subprocess

class PrivacyError(Exception):
    pass

def _ps(script: str, path: Path):
    env = dict(os.environ, MG_ACL_PATH=str(path.resolve()))
    # Do not inherit PowerShell7's module roots into Windows PowerShell5.
    env.pop("PSModulePath",None)
    r = subprocess.run(["powershell.exe", "-NoProfile", "-NonInteractive", "-Command", script],
                       env=env, capture_output=True, text=True, timeout=30)
    if r.returncode:
        raise PrivacyError("PRIVATE_ACL_UNAVAILABLE")
    return r.stdout

def verify(path: Path):
    if path.is_symlink() or path.resolve()!=path.absolute() or not path.exists():
        raise PrivacyError("PRIVATE_PATH_INVALID")
    if os.name == "nt":
        script = r"""
$ErrorActionPreference='Stop'
$p=$env:MG_ACL_PATH
$item=Get-Item -LiteralPath $p
if (($item.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) { exit 3 }
$acl=if ($item.PSIsContainer) { [IO.Directory]::GetAccessControl($p) } else { [IO.File]::GetAccessControl($p) }
$me=[Security.Principal.WindowsIdentity]::GetCurrent().User.Value
$allowed=@($me,'S-1-5-18','S-1-5-32-544')
if (!$acl.AreAccessRulesProtected -or $acl.GetOwner([Security.Principal.SecurityIdentifier]).Value -ne $me) { exit 3 }
$mine=$false
foreach ($rule in $acl.GetAccessRules($true,$true,[Security.Principal.SecurityIdentifier])) {
 if ($rule.AccessControlType -eq [Security.AccessControl.AccessControlType]::Allow) {
  if ($allowed -notcontains $rule.IdentityReference.Value) { exit 3 }
  if ($rule.IdentityReference.Value -eq $me -and
      (($rule.FileSystemRights -band [Security.AccessControl.FileSystemRights]::ReadData) -ne 0)) { $mine=$true }
 }
}
if (!$mine) { exit 3 }
Write-Output 'PRIVATE'
"""
        if _ps(script, path).strip() != "PRIVATE":
            raise PrivacyError("PRIVATE_ACL_REQUIRED")
    else:
        s = path.stat()
        if s.st_uid != os.getuid() or stat.S_IMODE(s.st_mode) & 0o077:
            raise PrivacyError("PRIVATE_MODE_REQUIRED")

def lock(path: Path):
    if path.is_symlink() or path.resolve()!=path.absolute() or not path.exists():
        raise PrivacyError("PRIVATE_PATH_INVALID")
    if os.name == "nt":
        script = r"""
$ErrorActionPreference='Stop'
$p=$env:MG_ACL_PATH
$item=Get-Item -LiteralPath $p
if (($item.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) { exit 3 }
$me=[Security.Principal.WindowsIdentity]::GetCurrent().User
$acl=if ($item.PSIsContainer) { New-Object Security.AccessControl.DirectorySecurity } else { New-Object Security.AccessControl.FileSecurity }
$acl.SetOwner($me)
$acl.SetAccessRuleProtection($true,$false)
$inherit=if ($item.PSIsContainer) { [Security.AccessControl.InheritanceFlags]'ContainerInherit,ObjectInherit' } else { [Security.AccessControl.InheritanceFlags]::None }
foreach ($sid in @($me.Value,'S-1-5-18','S-1-5-32-544')) {
 $id=New-Object Security.Principal.SecurityIdentifier($sid)
 $rule=New-Object Security.AccessControl.FileSystemAccessRule($id,'FullControl',$inherit,'None','Allow')
 $acl.AddAccessRule($rule)
}
if ($item.PSIsContainer) { [IO.Directory]::SetAccessControl($p,$acl) } else { [IO.File]::SetAccessControl($p,$acl) }
"""
        _ps(script, path)
    else:
        os.chmod(path, 0o700 if path.is_dir() else 0o600)
    verify(path)

def directory(path: Path):
    if path.exists():
        raise PrivacyError("PRIVATE_DESTINATION_EXISTS")
    path.mkdir(parents=True, exist_ok=False)
    lock(path)  # no private bytes have been written yet

def write_new(path: Path, data: bytes):
    verify(path.parent)
    fd = os.open(path, os.O_CREAT | os.O_EXCL | os.O_WRONLY, 0o600)
    try:
        # Empty file may inherit first; protect and verify *before* any bytes.
        lock(path)
        with os.fdopen(fd, "wb") as f:
            fd = -1
            f.write(data)
            f.flush()
            os.fsync(f.fileno())
        verify(path)
    finally:
        if fd >= 0:
            os.close(fd)

def verify_tree(root: Path):
    verify(root)
    for p in root.rglob("*"):
        verify(p)

def protect_generated(root: Path):
    # Generator writes only into an already restrictive directory.
    verify(root)
    for p in root.rglob("*"):
        lock(p)
    verify_tree(root)
