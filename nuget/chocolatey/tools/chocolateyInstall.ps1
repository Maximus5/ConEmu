$package = 'ConEmu'
$version = '19.10.12'
$sha256 = '0CF86BD9D62F0C54C2DC4F03B93717E2553A971924AC3DB21DEBC7D577DACD56'

$isSytem32Bit = (($Env:PROCESSOR_ARCHITECTURE -eq 'x86') -and `
  ($Env:PROCESSOR_ARCHITEW6432 -eq $null))

$os = if ($isSytem32Bit) { "x86" } else { "x64" }

$url = "https://github.com/Maximus5/ConEmu/releases/download/v$version/ConEmuSetup.$($version.replace('.','')).exe"

# MSI installer, but packed inside wrapper to select x86 or x64
# version. Therefore, treat it as EXE type.
$params = @{
  PackageName = $package;
  FileType = 'exe';
  SilentArgs = "/p:$os /quiet /norestart";
  Url = $url;
  Url64bit = $url;
  checksum      = $sha256
  checksumType  = 'sha256'
  checksum64    = $sha256
  checksumType64= 'sha256'
}

$script:CE_PID = 0
$script:CE_SRV = 0
$script:CE_WND = 0
$script:CE_DIR = ""

$script:ConEmuC = ""

$script:CE_FILES = @()

function log($s) {
  # Write-Host $s
}

function initWinApi() {
    Add-Type @"
      using System;
      using System.Text;
      using System.Runtime.InteropServices;
      public class ConEmuApi
      {
        [DllImport("user32.dll", SetLastError=true, CharSet=CharSet.Auto)]
        public static extern int IsWindow(IntPtr hwnd);

        [DllImport("kernel32.dll", SetLastError = true, CharSet = CharSet.Unicode)]
        static extern bool MoveFileEx(string lpExistingFileName, string lpNewFileName, int dwFlags);

        public static bool MarkFileDelete(string sourcefile)
        {
          return MoveFileEx(sourcefile, null, 4);
        }
      }
"@
}

function isWindowAlive($hwnd) {
  if (("ConEmuApi" -as [type]) -eq $null) {
    initWinApi
  }

  $rc = [ConEmuApi]::IsWindow($hwnd)
  # log "HWND=$hwnd, rc=$rc"
  return ($rc -ne 0)
}

function isProcessAlive($id) {
  if ((Get-Process -Id $id -ErrorAction Ignore) -eq $Null) {
    return $False
  }
  return $True
}

function isConEmu() {
  if (($env:ConEmuDrawHWND -eq $Null) -Or ($env:ConEmuPID -eq $Null) -Or
      ($env:ConEmuServerPID -eq $Null) -Or ($env:ConEmuBaseDir -eq $Null)) {
    return $False
  }

  $id = [convert]::ToInt32($env:ConEmuPID)
  $srv = [convert]::ToInt32($env:ConEmuServerPID)
  $wnd = [convert]::ToInt32($env:ConEmuDrawHWND, 16)

  if (-Not (isProcessAlive $id)) {
    return $False
  }

  if (-Not (isWindowAlive $wnd)) {
    return $False
  }

  if (Test-Path -path (Join-Path $env:ConEmuBaseDir "ConEmuC.exe") -pathtype leaf) {
    $script:ConEmuC = Join-Path $env:ConEmuBaseDir "ConEmuC.exe"
  } elseif (Test-Path -path (Join-Path $env:ConEmuBaseDir "ConEmuC64.exe") -pathtype leaf) {
    $script:ConEmuC = Join-Path $env:ConEmuBaseDir "ConEmuC64.exe"
  } else {
    return $False
  }

  $script:CE_PID = $id
  $script:CE_SRV = $srv
  $script:CE_WND = $wnd
  $script:CE_DIR = $env:ConEmuBaseDir
  return $True
}

function unlockFiles($files) {
  $suffix = Get-Date -format yyMMddHHmmss
  foreach ($fn in $files) {
    $f = @{src_name=$fn; tmp_name=".$fn.$suffix";}
    $f.Add("src", (Join-Path $script:CE_DIR $f.src_name))
    $f.Add("tmp", (Join-Path $script:CE_DIR $f.tmp_name))
    try {
      if (Test-Path -path $f.src -pathtype leaf) {
        Rename-Item -Path $f.src -NewName $f.tmp_name -ErrorAction Stop
        $script:CE_FILES += $f
      }
    } catch {
    }
  }
}

function removeTempFiles() {
  foreach ($f in $script:CE_FILES) {
    try {
      Remove-Item $f.tmp -ErrorAction Stop
    } catch {
      $deleteResult = [ConEmuApi]::MarkFileDelete($f.tmp)
    }
  }
}

function restoreFiles() {
  foreach ($f in $script:CE_FILES) {
    try {
      Rename-Item -Path $f.tmp -NewName $f.src_name -ErrorAction Stop
    } catch {
      log "Failed to restore file: $($f.src_name)"
    }
  }
}

function ConEmuPreUpdate() {
  # Checking
  if (isConEmu) {
    # Note, old ConEmu versions weren't able to detach without confirmation
    log "Detaching from ConEmu"
    $warn_msg = "Please choose <Yes> in the confirmation dialog"
    Write-Host -ForegroundColor Green -NoNewLine $warn_msg
    $macro_rc = & $script:ConEmuC -GuiMacro:$script:CE_PID detach 3
    Write-Host -NoNewLine ("`r".PadRight($warn_msg.length+1)+"`r")

    $wait_delay = 10000
    $wait_ms = 250
    $wait_count = $wait_delay/$wait_ms

    # log "Code=$LASTEXITCODE" # may be always 0 and "OK"
    # Sleep for 10 sec waiting for detach
    $warn_msg = "Waiting for detach"
    Write-Host -ForegroundColor Green -NoNewLine $warn_msg
    for ($i = 0; $i -lt $wait_count; $i++) {
      if (-Not (isWindowAlive $script:CE_WND)) {
        log "Detached successfully"
        break
      }
      # Write-Host -NoNewLine "."
      Sleep -m $wait_ms
    }
    Write-Host -NoNewLine ("`r".PadRight($warn_msg.length+1)+"`r")
    if ($i -eq $wait_count) {
      throw "Detach failed, please try to run update outside of ConEmu"
    }
    

    if (isProcessAlive $script:CE_PID) {
      Sleep -s 1
      if (isProcessAlive $script:CE_PID) {
        $warn_msg = "Requesting ConEmu termination"
        Write-Host -ForegroundColor Green -NoNewLine $warn_msg
        $macro_rc = & $script:ConEmuC -GuiMacro:$script:CE_PID close 2
        for ($i = 0; $i -lt $wait_count; $i++) {
          if (-Not (isProcessAlive $script:CE_PID)) {
            log "Terminated successfully"
            break
          }
          # Write-Host -NoNewLine "."
          Sleep -m $wait_ms
        }
        Write-Host -NoNewLine ("`r".PadRight($warn_msg.length+1)+"`r")
        if ($i -eq $wait_count) {
          throw "ConEmu process still alive, please try to run update outside of ConEmu"
        }
      }
    }

    if (isProcessAlive $script:CE_SRV) {
      log "Terminating console server process PID=$($script:CE_SRV)"
      Stop-Process -Force -Id $script:CE_SRV
    }

    log "Renaming hooks libraries"
    unlockFiles @("ConEmuHk.dll", "ConEmuHk64.dll")

    log "Console is ready to update ConEmu"

  } else {
    $is_conemu = $FALSE
    try {
      & ConEmuC.exe -IsConEmu
      $is_conemu = ($LASTEXITCODE -eq 1)
    } catch {
    }
    if ($is_conemu) {
      throw "Can't detect proper ConEmu environment variables! Please restart console!"
    }

    log "PowerShell was not stared from ConEmu"
  }
}

function ConEmuPostUpdate() {
  if ($script:CE_FILES.Count -ne 0) {
    if (Test-Path -path $script:CE_FILES[0] -pathtype leaf) {
      # On Success
      removeTempFiles
    } else {
      # On Failure
      restoreFiles
    }
  }
  if ($script:CE_PID -ne 0) {
    Write-Host -ForegroundColor Green "Console restart is strongly recommended to gain proper ConEmu environment"
    # Reattach
    $rc = Start-Process -FilePath $script:ConEmuC -ArgumentList @("/ATTACH","/NOCMD") -NoNewWindow
    for ($i = 0; $i -lt 100; $i++) {
      $rc = & $script:ConEmuC -IsConEmu
      if ($LASTEXITCODE -eq 1) { break }
      Sleep -m 100
    }
  }
}

# Unlock ConEmu binaries
ConEmuPreUpdate

# Do installation/update
Install-ChocolateyPackage @params

# Try to restore files if update has failed
ConEmuPostUpdate

# Done
